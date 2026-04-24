#include "my_math.h"

#define MIN_F 1e-6

int Max(int a,int b)
{
	return (a > b) ? a : b;
}

float fMax(float a,float b)
{
	 return (a > b) ? a : b;
}

int Min(int a,int b)
{
	return (a < b) ? a : b;
}

float fMin(float a,float b)
{
	return (a < b) ? a : b;
}

float fXianfu(float num,float bottom,float top) //�����޷�
{
	if(num>top)
	{
		return top;
	}
	else if(num<bottom)
	{
		return bottom;
	}
	else
	{
		return num;
	}
}

int Xianfu(int num,int bottom,int top) //�����޷�
{
	if(num>top)
	{
		return top;
	}
	else if(num<bottom)
	{
		return bottom;
	}
	else
	{
		return num;
	}
}

float fGuiLing(float num,float bottom,float top)
{
	if(num<top && num>bottom)
	{
		return 0.0;
	}
	else
	{
		return num;
	}
}

int GuiLing(int num,int bottom,int top)
{
	if(num<top && num>bottom)
	{
		return 0.0;
	}
	else
	{
		return num;
	}
}