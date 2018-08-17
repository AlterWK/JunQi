/*
 * communication.c
 *
 *  Created on: Aug 15, 2018
 *      Author: Administrator
 */

#include "comm.h"
#include "pthread.h"
#include "junqi.h"
#include "rule.h"


void PacketHeader(CommHeader *header, u8 iDir, u8 eFun)
{
	memset(header, 0, sizeof(CommHeader));
	memcpy(header->aMagic, aMagic, 4);
	//目前该位只在COMM_START标识先手位
	header->iDir = iDir;
	header->eFun = eFun;
}

void SendData(Junqi* pJunqi, CommHeader *header, void *data, int len)
{
	u8 buf[100];
	int length = 0;

	length += sizeof(CommHeader);
	memcpy(buf, header, length);

	memcpy(buf+length, data, len);
	length += len;

	sendto(pJunqi->socket_fd, buf, length, 0,
			(struct sockaddr *)&pJunqi->addr, sizeof(struct sockaddr));
}

void SendHeader(Junqi* pJunqi, u8 iDir, u8 eFun)
{
	CommHeader header;
	PacketHeader(&header, iDir, eFun);
	SendData(pJunqi, &header, NULL, 0);
}

void GetSendLineup(Junqi* pJunqi, u8 *data, int iDir)
{
	int i;
	for(i=0; i<30; i++)
	{
		data[i] = pJunqi->Lineup[iDir][i].type;
	}

}

void SendLineup(Junqi* pJunqi, int iDir)
{
	CommHeader header;
	u8 aData[30];
	if( iDir%2!=0 )
	{
		PacketHeader(&header, iDir, COMM_LINEUP);
		GetSendLineup(pJunqi, aData, iDir);
		SendData(pJunqi, &header, aData, 30);
	}
}

void SendMoveResult(Junqi* pJunqi, int iDir, MoveResultData *pData)
{
	CommHeader header;
	PacketHeader(&header, iDir, COMM_MOVE);
	SendData(pJunqi, &header, pData, sizeof(MoveResultData));
}

void SendEvent(Junqi* pJunqi, int iDir, u8 event)
{
	CommHeader header;
	u8 data = event;
	PacketHeader(&header, iDir, COMM_EVNET);
	SendData(pJunqi, &header, &data, 1);
}

void DealRecData(Junqi* pJunqi, u8 *data)
{
	CommHeader *pHead;
	pHead = (CommHeader *)data;
	BoardChess *pSrc, *pDst;
	BoardPoint p1,p2;
	u8 event;


	if( memcmp(pHead->aMagic, aMagic, 4)!=0 )
	{
		return;
	}

	switch(pHead->eFun)
	{
	case COMM_READY:
		PacketHeader(pHead, 0, COMM_INIT);
		data = (u8*)&pHead[1];
		memset(data, 0, 64);//前4个字节表示要发送哪一家布阵
		data[1] = data[3] = 1;
		GetSendLineup(pJunqi, data+4, 1);
		GetSendLineup(pJunqi, data+34, 3);
		SendData(pJunqi, pHead, data, 64);
		break;
	case COMM_MOVE:
		data = (u8*)&pHead[1];
		p1.x = data[0]%17;
		p1.y = data[1]%17;
		p2.x = data[2]%17;
		p2.y = data[3]%17;
		if( pJunqi->bStart && pJunqi->eTurn==pHead->iDir && pHead->iDir%2==1 )
		{
			if( pJunqi->aBoard[p1.x][p1.y].pAdjList && pJunqi->aBoard[p2.x][p2.y].pAdjList )
			{
				pSrc = pJunqi->aBoard[p1.x][p1.y].pAdjList->pChess;
				pDst = pJunqi->aBoard[p2.x][p2.y].pAdjList->pChess;
				if( pSrc==NULL || pDst==NULL )
				{
					SendHeader(pJunqi, pHead->iDir, COMM_ERROR);
					return;
				}
			}
			else
			{
				SendHeader(pJunqi, pHead->iDir, COMM_ERROR);
				return;
			}

			if( pSrc->pLineup->iDir==pHead->iDir && IsEnableMove(pJunqi, pSrc, pDst) )
			{
				gtk_widget_hide(pJunqi->redRectangle[0]);
				gtk_widget_hide(pJunqi->redRectangle[1]);

				int type;
				type = CompareChess(pSrc, pDst);
				PlayResult(pJunqi, pSrc, pDst, type);
				ChessTurn(pJunqi);
			}
			else
			{
				SendHeader(pJunqi, pHead->iDir, COMM_ERROR);
			}
		}
		break;
	case COMM_EVNET:
		event = *((u8*)&pHead[1]);
		if( event==JUMP_EVENT )
		{
			if( pHead->iDir==pJunqi->eTurn )
			{
				AddEventToReplay(pJunqi, JUMP_EVENT, pHead->iDir);
				IncJumpCnt(pJunqi, pHead->iDir);
				ChessTurn(pJunqi);
				SendEvent(pJunqi, pHead->iDir, JUMP_EVENT);
			}
		}
		else if( event==SURRENDER_EVENT )
		{
			if( pHead->iDir==pJunqi->eTurn )
			{
				SendEvent(pJunqi, pHead->iDir, SURRENDER_EVENT);
				SendSoundEvent(pJunqi,DEAD);
				DestroyAllChess(pJunqi, pHead->iDir);
				AddEventToReplay(pJunqi, SURRENDER_EVENT, pHead->iDir);
				ChessTurn(pJunqi);
			}
		}
		else
		{
			SendHeader(pJunqi, pHead->iDir, COMM_ERROR);
		}
		break;
	case COMM_LINEUP:
		if( pHead->iDir%2==1 )
		{
			LoadLineup(pJunqi, pHead->iDir, (u8*)&pHead[1]);
		}
		break;
	default:
		break;
	}

}
void *comm_thread(void *arg)
{
	Junqi* pJunqi = (Junqi*)arg;
	int socket_fd;
	struct sockaddr_in addr,local;
	size_t recvbytes = 0;
	u8 buf[200]={0};

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0)
	{
		printf("Create Socket Failed!\n");
		pthread_detach(pthread_self());
	}

	local.sin_family = AF_INET;
	local.sin_addr.s_addr=INADDR_ANY;
	local.sin_port = htons(1234);
	if(bind(socket_fd, (struct sockaddr *)&local, sizeof(struct sockaddr) )<0)
	{
		printf("Bind Error!\n");
		pthread_detach(pthread_self());
	}

	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(5678);

	pJunqi->socket_fd = socket_fd;
	pJunqi->addr = addr;

	SendHeader(pJunqi, pJunqi->eTurn, COMM_READY);

	while(1)
	{
		recvbytes=recvfrom(socket_fd, buf, 200, 0,NULL ,NULL);
		(void)recvbytes;//暂时不用，消除告警
		DealRecData(pJunqi, buf);
	}

	pthread_detach(pthread_self());
	return NULL;

}


void CreatCommThread(Junqi* pJunqi)
{
    pthread_t tidp;
    pthread_create(&tidp,NULL,(void*)comm_thread,pJunqi);
}



