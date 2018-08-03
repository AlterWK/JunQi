/*
 * board.h
 *
 *  Created on: Jul 31, 2018
 *      Author: Administrator
 */

#ifndef BOARD_H_
#define BOARD_H_

//���ӷ���һ�������εĸ�������ﶨ��߳�
//�Լ���ԼҸ��ӵĳ�Ҫ��΢��һ�㣬��40
#define LENGTH1   39
#define LENGTH2   40

//�ֱ�Ϊ�ϼҡ��Լҡ��¼����Ͻǵĺ�����
#define LEFTX     38
#define MIDX      265
#define RIGHTX    467

//�ֱ�Ϊ�Լҡ��ϼҡ��Լ����Ͻǵ�������
#define TOPY      13
#define MIDY      242
#define BOTTOMY   442

//�Ź������Ͻǵĺ�������
#define NINE_GRID_X   MIDX
#define NINE_GRID_Y   (TOPY+6*LENGTH1)

//ÿ�ҷ���ĳ��Ϳ��Լ��ͶԼҿ��Ҫ��΢��һ��
#define HORIZONTAL_LONG1  198 //5*40-2
#define HORIZONTAL_LONG2  191 //5*39-2
#define VERTICAL_LONG     232 //6*39-2

#define RECTANGLE_WHITE 0
#define RECTANGLE_RED 1

GtkWidget *GetSelectImage(int isVertical, int color);
GdkPixbuf* get_from_pixbuf_position(GdkPixbuf* pixbuf,
								gint src_x,
								gint src_y,
								gint width,
								gint height );

#endif /* BOARD_H_ */
