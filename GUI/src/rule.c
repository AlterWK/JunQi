/*
 * rule.c
 *
 *  Created on: Aug 4, 2018
 *      Author: Administrator
 */
#include "junqi.h"
#include "board.h"

int malloc_cnt = 0;
int free_cnt = 0;

int IsChangeValid(BoardChess *pSrc, BoardChess *pDst)
{
	int rc = 1;

	if( pSrc->type==ZHADAN )
	{
		if( pDst->index<5 )
		{
			rc = 0;
		}
	}
	else if( pSrc->type==DILEI )
	{
		if( pDst->index<20 )
		{
			rc = 0;
		}
	}
	else if( pSrc->type==JUNQI )
	{
		if( pDst->index!=26 && pDst->index!=28 )
		{
			rc = 0;
		}
	}

    return rc;
}


int IsEnableChange(BoardChess *pSrc, BoardChess *pDst)
{
	int rc = 1;

	if( (rc = IsChangeValid(pSrc, pDst)) )
	{
		rc = IsChangeValid(pDst, pSrc);
	}

    return rc;
}

void ClearPassCnt(Junqi *pJunqi)
{
	int i,j;

	pJunqi->szPath = 0;
	for(i=0; i<17; i++)
	{
		for(j=0; j<17; j++)
		{
			pJunqi->aBoard[i][j].passCnt = 0;
		}
	}
}

void ClearPathArrow(Junqi *pJunqi, int iPath)
{
	GraphPath *p;
	GraphPath *pTmp;

	if( pJunqi->pPath[iPath]==NULL )
	{
		return;
	}
	assert( pJunqi->pPath[iPath]->isHead );
	p = pJunqi->pPath[iPath]->pNext;
	while(1)
	{

		log_a("remove %d : %d %d",iPath,p->pChess->point.x,p->pChess->point.y);
		if(p->isHead)
		{
			free_cnt++;
			log_a("free %d",free_cnt);
			gtk_widget_destroy(p->pArrow);
			g_object_unref (p->pArrow);
			free(p);
			break;
		}
		else
		{
			free_cnt++;
			log_a("free %d",free_cnt);
			gtk_widget_destroy(p->pArrow);
			g_object_unref (p->pArrow);
			pTmp = p;
			p = p->pNext;
			free(pTmp);
		}

	}

	pJunqi->pPath[iPath] = NULL;
}



GtkWidget* GetArrowImage(Junqi *pJunqi, BoardChess *pSrc, BoardChess *pDst)
{
	//a[1][1]不用
	u8 aMap[3][3]={
		{3, 4, 5},
		{2, 8, 6},
		{1, 0, 7}
	};
	int iArrow;
	int x,y;
	GdkPixbuf *pixbuf;
	GtkWidget* image;

	x = pDst->point.x-pSrc->point.x;
	if( x>0 ) x=1;
	else if( x<0 ) x=-1;
	x = x+1;

	y = pDst->point.y-pSrc->point.y;
	if( y>0 ) y=1;
	else if( y<0 ) y=-1;
	y = y+1;

    assert( x!=1 || y!=1 );
	iArrow = aMap[x][y];
	pixbuf = pJunqi->paArrowPixbuf[iArrow];
	image = gtk_image_new_from_pixbuf(pixbuf);

	return image;
}

void AddPathArrow(Junqi *pJunqi, BoardChess *pSrc, BoardChess *pDst, int iPath)
{
	GraphPath *p;
	GraphPath *pHead;
	pHead= pJunqi->pPath[iPath];
	p = (GraphPath *)malloc(sizeof(GraphPath));
	memset(p, 0, sizeof(GraphPath));
    p->pChess = pSrc;
    p->pArrow = GetArrowImage(pJunqi, pSrc, pDst);
    g_object_ref_sink (p->pArrow);
	if( pHead==NULL )
	{
		p->isHead = 1;
		p->pNext = p;
		p->pPrev = p;
        pJunqi->pPath[iPath] = p;
	}
	else
	{
		//插入尾部
		p->pNext = pHead;
		p->pPrev = pHead->pPrev;
		pHead->pPrev = p;
		p->pPrev->pNext = p;
	}
	log_a("add %d : %d %d dst %d %d",iPath,
			p->pChess->point.x,p->pChess->point.y,
			pDst->point.x,pDst->point.y);

	malloc_cnt++;
	log_a("malloc %d",malloc_cnt);

}

void CopyPathArrow(Junqi *pJunqi, BoardChess *pDst, int iPath1, int iPath2)
{
	GraphPath *p;

	assert( pJunqi->pPath[iPath1]==NULL );
	assert( pJunqi->pPath[iPath2]!=NULL );

	p = pJunqi->pPath[iPath2];
	assert( p->isHead );

	do
	{
		if( !p->pNext->isHead )
		{
			AddPathArrow(pJunqi, p->pChess, p->pNext->pChess, iPath1);
		}
		else
		{
			AddPathArrow(pJunqi, p->pChess, pDst, iPath1);
			break;
		}

		p = p->pNext;
	}while(1);
}


void RemovePathTail(Junqi *pJunqi, int iPath)
{
	GraphPath *pTail;
	GraphPath *pHead;

	pHead = pJunqi->pPath[iPath];
	if( pHead==NULL)
	{
		return;
	}
	pTail = pHead->pPrev;
	log_a("remove %d : %d %d",iPath,pTail->pChess->point.x,pTail->pChess->point.y);
	gtk_widget_destroy(pTail->pArrow);
	g_object_unref (pTail->pArrow);
	if( !pTail->isHead )
	{
		pTail->pPrev->pNext = pHead;
		pHead->pPrev = pTail->pPrev;
		free(pTail);
	}
	else
	{
		free(pTail);
		pJunqi->pPath[iPath] = NULL;
	}

	free_cnt++;
	log_a("free %d",free_cnt);

}

u8 IsSameRail(
		Junqi *pJunqi,
		BoardGraph *pSrc,
		BoardGraph *pDst,
		enum RailType type)
{
	BoardChess *pSrcChess;
	BoardChess *pDstChess;
	u8 rc =0;

	pSrcChess = pSrc->pAdjList->pChess;
	pDstChess = pDst->pAdjList->pChess;

	switch(type)
	{
	case HORIZONTAL_RAIL:
		if( pSrcChess->point.x==pDstChess->point.x )
			rc = 1;
		break;
	case VERTICAL_RAIL:
		if( pSrcChess->point.y==pDstChess->point.y )
			rc = 1;
		break;
	case CURVE_RAIL:
		assert( pSrcChess->eCurveRail>0 );
		if( pSrcChess->eCurveRail==pDstChess->eCurveRail  )//&& pSrcChess->eCurveRail>0
			rc = 1;
		break;
	default:
		break;
	}

	return rc;
}

u8 GetRailPath(
		Junqi *pJunqi,
		BoardGraph *pSrc,
		BoardGraph *pDst,
		enum RailType type)
{
	AdjNode *p;
	BoardGraph *pVertex;
	u8 rc = 0;
	u8 pathFlag = 0;

	pSrc->passCnt++;

	for(p=pSrc->pAdjList->pNext; p!=NULL; p=p->pNext)
	{
		pVertex = &pJunqi->aBoard[p->pChess->point.x][p->pChess->point.y];
		if( p->pChess==pDst->pAdjList->pChess )
		{
			if(pathFlag)
			{
				RemovePathTail(pJunqi, 2);
			}
			AddPathArrow(pJunqi, pSrc->pAdjList->pChess, p->pChess, 2);
			if( pDst->passCnt==0 )
			{
				pDst->passCnt = pSrc->passCnt+1;
				ClearPathArrow(pJunqi, 1);
				CopyPathArrow(pJunqi, p->pChess, 1, 2);

			}
			else if( pSrc->passCnt+1<pDst->passCnt )
			{
				pDst->passCnt = pSrc->passCnt+1;
		    	ClearPathArrow(pJunqi, 1);
		    	CopyPathArrow(pJunqi, p->pChess, 1, 2);
#if 0//测试路径用
		    	GraphPath *p = pJunqi->pPath[1];
		    	log_a("path %d %d",p->pChess->point.x,p->pChess->point.y);
		    	for(p=p->pNext;!p->isHead;p=p->pNext)
		    	{
		    		log_a("list %d %d",p->pChess->point.x,p->pChess->point.y);
		    	}
#endif
			}
			pJunqi->szPath = pDst->passCnt;
			RemovePathTail(pJunqi, 2);

			return 1;
		}
		else if( p->pChess->type!=NONE )
		{
			continue;
		}
		else if( pVertex->passCnt!=0 && (pSrc->passCnt+1>=pVertex->passCnt) )
		{
			//从起点到该点的长度比之前遍历过的要大
			continue;
		}
		else if( type!=GONGB_RAIL && !IsSameRail(pJunqi, pSrc, pVertex, type))
		{
            continue;
		}
		else
		{
			if(pathFlag)
			{
				RemovePathTail(pJunqi, 2);
			}
			pVertex->passCnt = pSrc->passCnt;
			AddPathArrow(pJunqi, pSrc->pAdjList->pChess, p->pChess, 2);
			pathFlag = 1;
			rc |= GetRailPath(pJunqi, pVertex, pDst, type);
		}
	}
	if(pathFlag)
	{
		RemovePathTail(pJunqi, 2);
	}
	return rc;
}


int IsEnableMove(Junqi *pJunqi, BoardChess *pSrc, BoardChess *pDst, u8 isShowPath)
{
	int rc = 0;

	BoardGraph *pVertex1;
	BoardGraph *pVertex2;

	ClearPassCnt(pJunqi);

	if(pSrc->isStronghold)
	{
		return rc;
	}
	else if( pDst->isCamp && pDst->type!=NONE )
	{
		return rc;
	}
	else if( pSrc->type==DILEI || pSrc->type==JUNQI )
	{
		return rc;
	}


	//如果是相邻的格子（包括斜相邻）
	if( ((pDst->point.x-pSrc->point.x)>=-1 && (pDst->point.x-pSrc->point.x)<=1) &&
			((pDst->point.y-pSrc->point.y)>=-1 && (pDst->point.y-pSrc->point.y)<=1) )
	{
		//营与周围的格子都是相邻的
		if( pSrc->isCamp || pDst->isCamp )
		{
			rc = 1;
		}
		//非斜相邻
		else if( pDst->point.x==pSrc->point.x || pDst->point.y==pSrc->point.y)
		{
			rc = 1;
		}
		if(rc)
		{
			AddPathArrow(pJunqi, pSrc, pDst, 1);
		}
	}
	if( !rc && pSrc->isRailway && pDst->isRailway )
	{
		pVertex1 = &pJunqi->aBoard[pSrc->point.x][pSrc->point.y];
		pVertex2 = &pJunqi->aBoard[pDst->point.x][pDst->point.y];

		if( pSrc->type==GONGB )
		{
			rc = GetRailPath(pJunqi, pVertex1, pVertex2, GONGB_RAIL);
		}
		else if( pDst->point.x==pSrc->point.x )
		{
			rc = GetRailPath(pJunqi, pVertex1, pVertex2, HORIZONTAL_RAIL);
		}
		else if( pDst->point.y==pSrc->point.y )
		{
			rc = GetRailPath(pJunqi, pVertex1, pVertex2, VERTICAL_RAIL);
		}
		else if( pDst->eCurveRail>0 && pDst->eCurveRail==pSrc->eCurveRail )
		{
			rc = GetRailPath(pJunqi, pVertex1, pVertex2, CURVE_RAIL);
		}
	}
    assert( pJunqi->pPath[2] == NULL );
    assert( rc || pJunqi->pPath[1] == NULL );
    if( rc && isShowPath)
    {
    	ClearPathArrow(pJunqi, 0);
    	pJunqi->pPath[0] = pJunqi->pPath[1];
    	pJunqi->pPath[1] = NULL;
    }
    else
    {
    	ClearPathArrow(pJunqi, 1);
    	pJunqi->pPath[1] = NULL;
    }

	return rc;
}

int CompareChess(BoardChess *pSrc, BoardChess *pDst)
{
	enum CompareType result;
    assert( pSrc->type!=NONE );
    if( pDst->type==NONE )
    {
    	result = MOVE;
    }
    else if( pDst->type==ZHADAN||pSrc->type==ZHADAN )
    {
    	result = BOMB;
    }
    else if( pDst->type==JUNQI )
    {
    	result = EAT;
    }
    else if( pDst->type==DILEI )
    {
    	if( pSrc->type==GONGB )
    	{
    		result = EAT;
    	}
    	else
    	{
    		result = KILLED;
    	}
    }
    else
    {
    	assert( pDst->type>=SILING && pDst->type<=GONGB &&
    			pSrc->type>=SILING && pSrc->type<=GONGB );
    	if( pDst->type == pSrc->type )
    	{
    		result = BOMB;
    	}
    	else if( pSrc->type<pDst->type )
    	{
    		result = EAT;
    	}
    	else
    	{
    		result = KILLED;
    	}
    }

	return result;
}

void ChessTurn(Junqi *pJunqi)
{
	pJunqi->eTurn = (pJunqi->eTurn+1)%4;
	//下家阵亡
	if( pJunqi->aInfo[pJunqi->eTurn].bDead )
	{
		pJunqi->eTurn = (pJunqi->eTurn+1)%4;
		//下下家阵亡
		if( pJunqi->aInfo[pJunqi->eTurn].bDead )
		{
			pJunqi->eTurn = (pJunqi->eTurn+1)%4;
		}
	}

	pJunqi->bSelect = 0;
	gtk_widget_hide(pJunqi->whiteRectangle[0]);
	gtk_widget_hide(pJunqi->whiteRectangle[1]);
}

void IncJumpCnt(Junqi *pJunqi, int iDir)
{
	int cntJump;
	pJunqi->aInfo[iDir].cntJump++;
	cntJump = pJunqi->aInfo[iDir].cntJump;

	if( cntJump==5 )
	{
		SendSoundEvent(pJunqi,DEAD);
		DestroyAllChess(pJunqi, iDir);
		ClearChessFlag(pJunqi,iDir);

		HideJumpButton(iDir);
	}
    if( !pJunqi->bReplay )
    {
    	ShowDialogMessage(pJunqi, "跳过次数", cntJump);
    }
}

int IfHasMove(Junqi* pJunqi, BoardChess *pSrc)
{
	BoardChess *pTemp;
	int i;
	int rc = 0;


	for(i=0; i<129; i++)
	{
		if( i<120 )
		{
			pTemp = &pJunqi->ChessPos[i/30][i%30];
		}
		else
		{
			pTemp = &pJunqi->NineGrid[i-120];
		}
		if( pTemp->type!=NONE && pSrc->pLineup->iDir%2==pTemp->pLineup->iDir%2 )
		{
			continue;
		}
		if( IsEnableMove(pJunqi, pSrc,pTemp, 0) )
		{
			rc = 1;
			break;
		}
	}

	return rc;
}

int CheckIfDead(Junqi *pJunqi, int iDir)
{
	int rc = 1;
	int i;
	ChessLineup *pLineup;

	if( pJunqi->aInfo[iDir].bDead )
	{
		return 1;
	}

	for(i=0; i<30; i++)
	{
		pLineup = &pJunqi->Lineup[iDir][i];
		if( !pLineup->bDead && pLineup->type!=NONE )
		{
			assert(  aseertLineup(pLineup)  );
			if( IfHasMove(pJunqi, pLineup->pChess) )
			{
				rc = 0;
				break;
			}
		}
	}

	if( rc )
	{
		SendSoundEvent(pJunqi,DEAD);
		DestroyAllChess(pJunqi, iDir);
		ClearChessFlag(pJunqi,iDir);
		HideJumpButton(iDir);
		pJunqi->addr = pJunqi->addr_tmp[0];
		SendEvent(pJunqi, iDir, SURRENDER_EVENT);
		pJunqi->addr = pJunqi->addr_tmp[1];
		SendEvent(pJunqi, iDir, SURRENDER_EVENT);
	}

	return rc;
}
