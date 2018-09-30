/*
 * search.c
 *
 *  Created on: Sep 20, 2018
 *      Author: Administrator
 */
#include "engine.h"
#include "junqi.h"
#include "movegen.h"
#include "path.h"
#include "search.h"
#include "evaluate.h"
#include <time.h>

#include <omp.h>


#define OffSET(type,field) ((size_t)&(((type*)0)->field))
#define VAR_OFFSET 8

//int tt = 0;
void PushMoveToStack(
	Junqi *pJunqi,
	BoardChess *pSrc,
	BoardChess *pDst,
	MoveResultData *pMove )
{

	Engine *pEngine = pJunqi->pEngine;
	PositionList *p;
	PositionList *pHead;
	PositionData *pStorage;
	int iDir;

	p = (PositionList *)malloc(sizeof(PositionList));
	memset(p, 0, sizeof(PositionList));//��ʼ��
	pHead = pEngine->pPos;
	pStorage = &p->data;
	//log_c("make %d",++tt);
	if( pHead==NULL )
	{
		p->isHead = 1;
		p->pNext = p;
		p->pPre = p;
		pEngine->pPos = p;
	}
	else
	{
		p->pNext = pHead;
		p->pPre = pHead->pPre;
		pHead->pPre->pNext = p;
		pHead->pPre = p;
	}

	memcpy(&pStorage->xSrcChess, pSrc, VAR_OFFSET);
	memcpy(&pStorage->xDstChess, pDst, VAR_OFFSET);
	memcpy(&pStorage->xSrcLineup, pSrc->pLineup, sizeof(ChessLineup));


	//todo Ŀǰû�п���Ͷ����������������ߵ����
	if( pMove->result!=MOVE )
	{
		memcpy(&pStorage->xDstLineup, pDst->pLineup, sizeof(ChessLineup));
		if( pSrc->pLineup->iDir%2==ENGINE_DIR%2 )
		{
			iDir = pDst->pLineup->iDir;
		}
		else
		{
			iDir = pSrc->pLineup->iDir;
		}
		pStorage->junqi_chess_type[0] = pJunqi->ChessPos[iDir][26].type;
		pStorage->junqi_chess_type[1] = pJunqi->ChessPos[iDir][28].type;
		pStorage->junqi_type[0] = pJunqi->Lineup[iDir][26].type;
		pStorage->junqi_type[1] = pJunqi->Lineup[iDir][28].type;
		memcpy(&pStorage->enemyInfo, &pJunqi->aInfo[iDir], sizeof(PartyInfo));
        for(int i=0; i<30; i++)
        {
        	pStorage->mx_type[i] = pJunqi->Lineup[iDir][i].mx_type;
        }
	}
}

void PopMoveFromStack(
	Junqi *pJunqi,
	BoardChess *pSrc,
	BoardChess *pDst,
	MoveResultData *pMove )
{
	Engine *pEngine = pJunqi->pEngine;
	PositionList *pHead;
	PositionData *pStorage;
	PositionList *pTail;

	int iDir,iDir1;

	pHead = pEngine->pPos;
	pTail = pHead->pPre;
	pStorage = &pTail->data;

	assert( pHead!=NULL );

	memcpy(pSrc, &pStorage->xSrcChess, VAR_OFFSET);
	memcpy(pDst, &pStorage->xDstChess, VAR_OFFSET);
	memcpy(pSrc->pLineup, &pStorage->xSrcLineup, sizeof(ChessLineup));


	if( pMove->result!=MOVE )
	{
		memcpy(pDst->pLineup, &pStorage->xDstLineup, sizeof(ChessLineup));

		if( pSrc->pLineup->iDir%2==ENGINE_DIR%2 )
		{
			iDir = pDst->pLineup->iDir;
		}
		else
		{
			iDir = pSrc->pLineup->iDir;
		}

		pJunqi->Lineup[iDir][26].type = pStorage->junqi_type[0];
		pJunqi->Lineup[iDir][28].type = pStorage->junqi_type[1];
		pJunqi->ChessPos[iDir][26].type = pStorage->junqi_chess_type[0];
		pJunqi->ChessPos[iDir][28].type = pStorage->junqi_chess_type[1];


        for(int i=0; i<30; i++)
        {
        	pJunqi->Lineup[iDir][i].mx_type = pStorage->mx_type[i];
        }

    	//��ʱ������������ߵ����
    	iDir1 = pDst->pLineup->iDir;
    	if( pJunqi->aInfo[iDir1].bDead )
    	{
    		ChessLineup *pLineup;
    		for(int i=0; i<30; i++)
    		{
    			pLineup = &pJunqi->Lineup[iDir1][i];
    			if( !pLineup->bDead && pLineup->type!=NONE )
    			{
    				pLineup->pChess->type = pLineup->type;
    			}
    		}
    		pJunqi->aInfo[iDir1].bDead = 0;
    	}
    	//pJunqi->aInfo��Ҫ��֮ǰ��ԭ����ΪҪ�õ��ж��Ƿ�����
    	memcpy(&pJunqi->aInfo[iDir], &pStorage->enemyInfo, sizeof(PartyInfo));

	}

	if( !pTail->isHead )
	{
		//log_c("unmake %d",tt--);
		pTail->pPre->pNext = pHead;
		pHead->pPre = pTail->pPre;
		free(pTail);
	}
	else
	{
//		if( tt==2 )
//		{
//			log_c("NULL unmake %d",tt);
//		}
//		tt--;
		free(pTail);
		pEngine->pPos = NULL;
	}
}

void MakeNextMove(Junqi *pJunqi, MoveResultData *pResult)
{
	BoardChess *pSrc, *pDst;
	BoardPoint p1,p2;

	p1.x = pResult->src[0]%17;
	p1.y = pResult->src[1]%17;
	p2.x = pResult->dst[0]%17;
	p2.y = pResult->dst[1]%17;
	pSrc = pJunqi->aBoard[p1.x][p1.y].pAdjList->pChess;
	pDst = pJunqi->aBoard[p2.x][p2.y].pAdjList->pChess;
	PushMoveToStack(pJunqi, pSrc, pDst, pResult);
	PlayResult(pJunqi, pSrc, pDst, pResult);
	ChessTurn(pJunqi);


}

void UnMakeMove(Junqi *pJunqi, MoveResultData *pResult)
{
	BoardChess *pSrc, *pDst;
	BoardPoint p1,p2;


	p1.x = pResult->src[0]%17;
	p1.y = pResult->src[1]%17;
	p2.x = pResult->dst[0]%17;
	p2.y = pResult->dst[1]%17;
	pSrc = pJunqi->aBoard[p1.x][p1.y].pAdjList->pChess;
	pDst = pJunqi->aBoard[p2.x][p2.y].pAdjList->pChess;
	PopMoveFromStack(pJunqi, pSrc, pDst, pResult);


}

void SetBestMove(Junqi *pJunqi, MoveResultData *pResult)
{
	BoardChess *pSrc, *pDst;
	BoardPoint p1,p2;
	Engine *pEngine;

	pEngine = pJunqi->pEngine;
	if( pResult!=NULL )
	{
		p1.x = pResult->src[0]%17;
		p1.y = pResult->src[1]%17;
		p2.x = pResult->dst[0]%17;
		p2.y = pResult->dst[1]%17;
		pSrc = pJunqi->aBoard[p1.x][p1.y].pAdjList->pChess;
		pDst = pJunqi->aBoard[p2.x][p2.y].pAdjList->pChess;
		pEngine->pBest[0] = pSrc;
		pEngine->pBest[1] = pDst;
	}

}

int TimeOut(Junqi *pJunqi)
{
	int rc = 0;
	int search_time;
	search_time = time(NULL)-pJunqi->begin_time;
	if( pJunqi->bMove || pJunqi->bGo || search_time>20 )
	{
		rc = 1;
	}

	return rc;
}
int AlphaBeta(
		Junqi *pJunqi,
		int depth,
		int alpha,
		int beta)
{
	MoveList *pHead;
	MoveList *p;
	MoveResultData *pData;
	MoveResultData *pBest = NULL;
	int val;
	int sum = 0;
	int k = 0;
	static int cnt = 0;
	int iDir = pJunqi->eTurn;

	if(!cnt)
	{
		pJunqi->begin_time = (unsigned int)time(NULL);
	}
	cnt++;
	//���������һ��ʱ��������ֵ
	if( depth==0 )
	{
		val = EvalSituation(pJunqi);
		pJunqi->test_num++;
		//val = 5;
		//EvalSituation������������۵ģ����ԶԷ��ķ�ֵӦȡ��ֵ
		if( iDir%2!=ENGINE_DIR%2 )
		{
			val = -val;
		}
		cnt--;
		return val;
	}
	//�����ŷ��б�
	pHead = GenerateMoveList(pJunqi, iDir);
	if( pHead!=NULL )
	{
		pBest = &pHead->move;
	}
	//�������ʱֱ��������һ��
	else
	{
		pJunqi->eTurn = iDir;
		ChessTurn(pJunqi);
    	if( iDir%2==pJunqi->eTurn%2 )
    	{
    		//�¼������ֵ��Լ���
    		val = AlphaBeta(pJunqi,depth-1,alpha,beta);
    	}
    	else
    	{
    		val = -AlphaBeta(pJunqi,depth-1,-beta,-alpha);
    	}
	}

    //����ÿһ���ŷ�
    for(p=pHead; pHead!=NULL; p=p->pNext)
    {
    	pJunqi->eTurn = iDir;
    	//ģ���ŷ�������ľ���
    	MakeNextMove(pJunqi,&p->move);
    	assert(pJunqi->pEngine->pPos!=NULL);
    	if( iDir%2==pJunqi->eTurn%2 )
    	{
    		//�¼������ֵ��Լ���
    		val = AlphaBeta(pJunqi,depth-1,alpha,beta);
    	}
    	else
    	{
    		val = -AlphaBeta(pJunqi,depth-1,-beta,-alpha);
    	}
    	//�Ѿ��泷�ص���һ��
    	assert(pJunqi->pEngine->pPos!=NULL);
    	UnMakeMove(pJunqi,&p->move);

		sum += val;
		k++;
		//�ŷ���ͬ�����о������ͬ��ȡƽ��ֵ
		val = sum/k;
    	if( !p->pNext->isHead )
    	{
    		pData = &p->pNext->move;
            //��һ���ŷ�
    		if( memcmp(&p->move, pData, 4) )
    		{
    			sum = 0;
    			k = 0;
    		}
    	}
        //�����ض�
    	if( val>=beta )
    	{
    		alpha = beta;
    		break;
    	}
    	//����alphaֵ
    	if( val>alpha )
    	{
    		pBest = &p->move;
    		alpha = val;
    	}
    	if( p->pNext->isHead )
    	{
    		break;
    	}
        //ʱ��������յ�goָ���������
    	if( TimeOut(pJunqi) )
    	{
    		break;
    	}
//    	if(cnt==1)
//    	log_c("src %d %d",p->move.src[0],p->move.src[1]);
    }

    cnt--;
    if( 0==cnt )
    {
    	cnt = 0;
    	SetBestMove(pJunqi,pBest);
    }
	ClearMoveList(pHead);
	return alpha;
}

u8 SendBestMove(Engine *pEngine)
{
	Junqi *pJunqi = pEngine->pJunqi;
	u8 rc = 0;

	if( pEngine->pBest[0]!=NULL && pEngine->pBest[1]!=NULL )
	{
		assert( IsEnableMove(pJunqi, pEngine->pBest[0], pEngine->pBest[1]) );
		SendMove(pJunqi, pEngine->pBest[0], pEngine->pBest[1]);
		pEngine->pBest[0] = NULL;
		pEngine->pBest[1] = NULL;
		rc = 1;
	}


	return rc;
}
