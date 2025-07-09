#include "EPD_font.h"
#include "EPD_GUI.h"

PAINT Paint;

/*******************************************************************
		º¯ÊýËµÃ÷£º´´½¨Í¼Æ¬»º´æÊý×é
		½Ó¿ÚËµÃ÷£º*image  Òª´«ÈëµÄÍ¼Æ¬Êý×é
			   Width  Í¼Æ¬¿í¶È
			   Heighe Í¼Æ¬³¤¶È
			   Rotate ÆÁÄ»ÏÔÊ¾·½Ïò
			   Color  ÏÔÊ¾ÑÕÉ«
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void Paint_NewImage(uint8_t *image, uint16_t Width, uint16_t Height, uint16_t Rotate, uint16_t Color)
{
	Paint.Image = 0x00;
	Paint.Image = image;
	Paint.color = Color;
	Paint.widthMemory = Width;
	Paint.heightMemory = Height;
	Paint.widthByte = (Width % 8 == 0) ? (Width / 8) : (Width / 8 + 1);
	Paint.heightByte = Height;
	Paint.rotate = Rotate;
	if (Rotate == 0 || Rotate == 180)
	{
		Paint.width = Height;
		Paint.height = Width;
	}
	else
	{
		Paint.width = Width;
		Paint.height = Height;
	}
}

/*******************************************************************
		º¯ÊýËµÃ÷£ºÑ¡Ôñ»­²¼
		½Ó¿ÚËµÃ÷£ºImage »­²¼Êý×éÃû
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void Paint_SelectImage(uint8_t *Image)
{
	Paint.Image = Image;
}

/*******************************************************************
		º¯ÊýËµÃ÷£ºÇå¿Õ»º³åÇø
		½Ó¿ÚËµÃ÷£ºColor  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void Paint_Clear(uint8_t Color)
{
	uint16_t X, Y;
	uint32_t Addr;
	for (Y = 0; Y < Paint.heightByte; Y++)
	{
		for (X = 0; X < Paint.widthByte; X++)
		{
			Addr = X + Y * Paint.widthByte; // 8 pixel =  1 byte
			Paint.Image[Addr] = Color;
		}
	}
}

/*******************************************************************
		º¯ÊýËµÃ÷£ºµãÁÁÒ»¸öÏñËØµã
		½Ó¿ÚËµÃ÷£ºXpoint ÏñËØµãx×ø±ê²ÎÊý
			  Ypoint ÏñËØµãY×ø±ê²ÎÊý
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void Paint_SetPixel(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
	uint16_t X, Y;
	uint32_t Addr;
	uint8_t Rdata;
	switch (Paint.rotate)
	{
	case 0:
		if (Xpoint >= 396)
		{
			Xpoint += 8;
		}
		X = Xpoint;
		Y = Ypoint;
		break;
	case 90:
		if (Ypoint >= 396)
		{
			Ypoint += 8;
		}
		X = Paint.widthMemory - Ypoint - 1;
		Y = Xpoint;
		break;
	case 180:
		if (Xpoint >= 396)
		{
			Xpoint += 8;
		}
		X = Paint.widthMemory - Xpoint - 1;
		Y = Paint.heightMemory - Ypoint - 1;
		break;

	case 270:
		if (Ypoint >= 396)
		{
			Ypoint += 8;
		}
		X = Ypoint;
		Y = Paint.heightMemory - Xpoint - 1;
		break;
	default:
		return;
	}
	Addr = X / 8 + Y * Paint.widthByte;
	Rdata = Paint.Image[Addr];
	if (Color == BLACK)
	{
		Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8)); // ½«¶ÔÓ¦Êý¾ÝÎ»ÖÃ0
	}
	else
	{
		Paint.Image[Addr] = Rdata | (0x80 >> (X % 8)); // ½«¶ÔÓ¦Êý¾ÝÎ»ÖÃ1
	}
}

/*******************************************************************
		º¯ÊýËµÃ÷£º»®Ïßº¯Êý
		½Ó¿ÚËµÃ÷£ºXstart ÏñËØxÆðÊ¼×ø±ê²ÎÊý
			  Ystart ÏñËØYÆðÊ¼×ø±ê²ÎÊý
							Xend   ÏñËØx½áÊø×ø±ê²ÎÊý
			  Yend   ÏñËØY½áÊø×ø±ê²ÎÊý
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_DrawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color)
{
	uint16_t Xpoint, Ypoint;
	int dx, dy;
	int XAddway, YAddway;
	int Esp;
	char Dotted_Len;
	Xpoint = Xstart;
	Ypoint = Ystart;
	dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
	dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;
	XAddway = Xstart < Xend ? 1 : -1;
	YAddway = Ystart < Yend ? 1 : -1;
	Esp = dx + dy;
	Dotted_Len = 0;
	for (;;)
	{
		Dotted_Len++;
		Paint_SetPixel(Xpoint, Ypoint, Color);
		if (2 * Esp >= dy)
		{
			if (Xpoint == Xend)
				break;
			Esp += dy;
			Xpoint += XAddway;
		}
		if (2 * Esp <= dx)
		{
			if (Ypoint == Yend)
				break;
			Esp += dx;
			Ypoint += YAddway;
		}
	}
}
/*******************************************************************
		º¯ÊýËµÃ÷£º»­¾ØÐÎº¯Êý
		½Ó¿ÚËµÃ÷£ºXstart ¾ØÐÎxÆðÊ¼×ø±ê²ÎÊý
			  Ystart ¾ØÐÎYÆðÊ¼×ø±ê²ÎÊý
							Xend   ¾ØÐÎx½áÊø×ø±ê²ÎÊý
			  Yend   ¾ØÐÎY½áÊø×ø±ê²ÎÊý
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
			  mode   ¾ØÐÎÊÇ·ñ½øÐÐÌî³ä
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_DrawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color, uint8_t mode)
{
	uint16_t i;
	if (mode)
	{
		for (i = Ystart; i < Yend; i++)
		{
			EPD_DrawLine(Xstart, i, Xend, i, Color);
		}
	}
	else
	{
		EPD_DrawLine(Xstart, Ystart, Xend, Ystart, Color);
		EPD_DrawLine(Xstart, Ystart, Xstart, Yend, Color);
		EPD_DrawLine(Xend, Yend, Xend, Ystart, Color);
		EPD_DrawLine(Xend, Yend, Xstart, Yend, Color);
	}
}
/*******************************************************************
		º¯ÊýËµÃ÷£º»­Ô²º¯Êý
		½Ó¿ÚËµÃ÷£ºX_Center Ô²ÐÄxÆðÊ¼×ø±ê²ÎÊý
			  Y_Center Ô²ÐÄY×ø±ê²ÎÊý
							Radius   Ô²ÐÎ°ë¾¶²ÎÊý
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
			  mode   Ô²ÐÎÊÇ·ñÌî³äÏÔÊ¾
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_DrawCircle(uint16_t X_Center, uint16_t Y_Center, uint16_t Radius, uint16_t Color, uint8_t mode)
{
	int Esp, sCountY;
	uint16_t XCurrent, YCurrent;
	XCurrent = 0;
	YCurrent = Radius;
	Esp = 3 - (Radius << 1);
	if (mode)
	{
		while (XCurrent <= YCurrent)
		{ // Realistic circles
			for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++)
			{
				Paint_SetPixel(X_Center + XCurrent, Y_Center + sCountY, Color); // 1
				Paint_SetPixel(X_Center - XCurrent, Y_Center + sCountY, Color); // 2
				Paint_SetPixel(X_Center - sCountY, Y_Center + XCurrent, Color); // 3
				Paint_SetPixel(X_Center - sCountY, Y_Center - XCurrent, Color); // 4
				Paint_SetPixel(X_Center - XCurrent, Y_Center - sCountY, Color); // 5
				Paint_SetPixel(X_Center + XCurrent, Y_Center - sCountY, Color); // 6
				Paint_SetPixel(X_Center + sCountY, Y_Center - XCurrent, Color); // 7
				Paint_SetPixel(X_Center + sCountY, Y_Center + XCurrent, Color);
			}
			if ((int)Esp < 0)
				Esp += 4 * XCurrent + 6;
			else
			{
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	}
	else
	{ // Draw a hollow circle
		while (XCurrent <= YCurrent)
		{
			Paint_SetPixel(X_Center + XCurrent, Y_Center + YCurrent, Color); // 1
			Paint_SetPixel(X_Center - XCurrent, Y_Center + YCurrent, Color); // 2
			Paint_SetPixel(X_Center - YCurrent, Y_Center + XCurrent, Color); // 3
			Paint_SetPixel(X_Center - YCurrent, Y_Center - XCurrent, Color); // 4
			Paint_SetPixel(X_Center - XCurrent, Y_Center - YCurrent, Color); // 5
			Paint_SetPixel(X_Center + XCurrent, Y_Center - YCurrent, Color); // 6
			Paint_SetPixel(X_Center + YCurrent, Y_Center - XCurrent, Color); // 7
			Paint_SetPixel(X_Center + YCurrent, Y_Center + XCurrent, Color); // 0
			if ((int)Esp < 0)
				Esp += 4 * XCurrent + 6;
			else
			{
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	}
}

/******************************************************************************
	  º¯ÊýËµÃ÷£ºÏÔÊ¾ºº×Ö´®
	  Èë¿ÚÊý¾Ý£ºx,yÏÔÊ¾×ø±ê
				*s ÒªÏÔÊ¾µÄºº×Ö´®
				sizey ×ÖºÅ
				color ÎÄ×ÖÑÕÉ«
	  ·µ»ØÖµ£º  ÎÞ
******************************************************************************/
void EPD_ShowChinese(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
	while (*s != 0)
	{
		if (sizey == 12)
			EPD_ShowChinese12x12(x, y, s, sizey, color);
		else if (sizey == 16)
			EPD_ShowChinese16x16(x, y, s, sizey, color);
		else if (sizey == 24)
			EPD_ShowChinese24x24(x, y, s, sizey, color);
		else if (sizey == 32)
			EPD_ShowChinese32x32(x, y, s, sizey, color);
		else
			return;
		s += 2;
		x += sizey;
	}
}

/******************************************************************************
	  º¯ÊýËµÃ÷£ºÏÔÊ¾µ¥¸ö12x12ºº×Ö
	  Èë¿ÚÊý¾Ý£ºx,yÏÔÊ¾×ø±ê
				*s ÒªÏÔÊ¾µÄºº×Ö
				sizey ×ÖºÅ
				color ÎÄ×ÖÑÕÉ«
	  ·µ»ØÖµ£º  ÎÞ
******************************************************************************/
void EPD_ShowChinese12x12(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
	uint8_t i, j;
	uint16_t k;
	uint16_t HZnum;		  // ºº×ÖÊýÄ¿
	uint16_t TypefaceNum; // Ò»¸ö×Ö·ûËùÕ¼×Ö½Ú´óÐ¡
	uint16_t x0 = x;
	TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
	HZnum = sizeof(tfont12) / sizeof(typFNT_GB12); // Í³¼Æºº×ÖÊýÄ¿
	for (k = 0; k < HZnum; k++)
	{
		if ((tfont12[k].Index[0] == *(s)) && (tfont12[k].Index[1] == *(s + 1)))
		{
			for (i = 0; i < TypefaceNum; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (tfont12[k].Msk[i] & (0x01 << j))
						Paint_SetPixel(x, y, color); // »­Ò»¸öµã
					x++;
					if ((x - x0) == sizey)
					{
						x = x0;
						y++;
						break;
					}
				}
			}
		}
		continue; // ²éÕÒµ½¶ÔÓ¦µãÕó×Ö¿âÁ¢¼´ÍË³ö£¬·ÀÖ¹¶à¸öºº×ÖÖØ¸´È¡Ä£´øÀ´Ó°Ïì
	}
}

/******************************************************************************
	  º¯ÊýËµÃ÷£ºÏÔÊ¾µ¥¸ö16x16ºº×Ö
	  Èë¿ÚÊý¾Ý£ºx,yÏÔÊ¾×ø±ê
				*s ÒªÏÔÊ¾µÄºº×Ö
				sizey ×ÖºÅ
				color ÎÄ×ÖÑÕÉ«
	  ·µ»ØÖµ£º  ÎÞ
******************************************************************************/
void EPD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
	uint8_t i, j;
	uint16_t k;
	uint16_t HZnum;		  // ºº×ÖÊýÄ¿
	uint16_t TypefaceNum; // Ò»¸ö×Ö·ûËùÕ¼×Ö½Ú´óÐ¡
	uint16_t x0 = x;
	TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
	HZnum = sizeof(tfont16) / sizeof(typFNT_GB16); // Í³¼Æºº×ÖÊýÄ¿
	for (k = 0; k < HZnum; k++)
	{
		if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1)))
		{
			for (i = 0; i < TypefaceNum; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (tfont16[k].Msk[i] & (0x01 << j))
						Paint_SetPixel(x, y, color); // »­Ò»¸öµã
					x++;
					if ((x - x0) == sizey)
					{
						x = x0;
						y++;
						break;
					}
				}
			}
		}
		continue; // ²éÕÒµ½¶ÔÓ¦µãÕó×Ö¿âÁ¢¼´ÍË³ö£¬·ÀÖ¹¶à¸öºº×ÖÖØ¸´È¡Ä£´øÀ´Ó°Ïì
	}
}

/******************************************************************************
	  º¯ÊýËµÃ÷£ºÏÔÊ¾µ¥¸ö24x24ºº×Ö
	  Èë¿ÚÊý¾Ý£ºx,yÏÔÊ¾×ø±ê
				*s ÒªÏÔÊ¾µÄºº×Ö
				sizey ×ÖºÅ
				color ÎÄ×ÖÑÕÉ«
	  ·µ»ØÖµ£º  ÎÞ
******************************************************************************/
void EPD_ShowChinese24x24(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
	uint8_t i, j;
	uint16_t k;
	uint16_t HZnum;		  // ºº×ÖÊýÄ¿
	uint16_t TypefaceNum; // Ò»¸ö×Ö·ûËùÕ¼×Ö½Ú´óÐ¡
	uint16_t x0 = x;
	TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
	HZnum = sizeof(tfont24) / sizeof(typFNT_GB24); // Í³¼Æºº×ÖÊýÄ¿
	for (k = 0; k < HZnum; k++)
	{
		if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s + 1)))
		{
			for (i = 0; i < TypefaceNum; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (tfont24[k].Msk[i] & (0x01 << j))
						Paint_SetPixel(x, y, color); // »­Ò»¸öµã
					x++;
					if ((x - x0) == sizey)
					{
						x = x0;
						y++;
						break;
					}
				}
			}
		}
		continue; // ²éÕÒµ½¶ÔÓ¦µãÕó×Ö¿âÁ¢¼´ÍË³ö£¬·ÀÖ¹¶à¸öºº×ÖÖØ¸´È¡Ä£´øÀ´Ó°Ïì
	}
}

/******************************************************************************
	  º¯ÊýËµÃ÷£ºÏÔÊ¾µ¥¸ö32x32ºº×Ö
	  Èë¿ÚÊý¾Ý£ºx,yÏÔÊ¾×ø±ê
				*s ÒªÏÔÊ¾µÄºº×Ö
				sizey ×ÖºÅ
				color ÎÄ×ÖÑÕÉ«
	  ·µ»ØÖµ£º  ÎÞ
******************************************************************************/
void EPD_ShowChinese32x32(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
	uint8_t i, j;
	uint16_t k;
	uint16_t HZnum;		  // ºº×ÖÊýÄ¿
	uint16_t TypefaceNum; // Ò»¸ö×Ö·ûËùÕ¼×Ö½Ú´óÐ¡
	uint16_t x0 = x;
	TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
	HZnum = sizeof(tfont32) / sizeof(typFNT_GB32); // Í³¼Æºº×ÖÊýÄ¿
	for (k = 0; k < HZnum; k++)
	{
		if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s + 1)))
		{
			for (i = 0; i < TypefaceNum; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (tfont32[k].Msk[i] & (0x01 << j))
						Paint_SetPixel(x, y, color); // »­Ò»¸öµã
					x++;
					if ((x - x0) == sizey)
					{
						x = x0;
						y++;
						break;
					}
				}
			}
		}
		continue; // ²éÕÒµ½¶ÔÓ¦µãÕó×Ö¿âÁ¢¼´ÍË³ö£¬·ÀÖ¹¶à¸öºº×ÖÖØ¸´È¡Ä£´øÀ´Ó°Ïì
	}
}

/*******************************************************************
		º¯ÊýËµÃ÷£ºÏÔÊ¾µ¥¸ö×Ö·û
		½Ó¿ÚËµÃ÷£ºx 		 ×Ö·ûx×ø±ê²ÎÊý
			  y 		 ×Ö·ûY×ø±ê²ÎÊý
							chr    ÒªÏÔÊ¾µÄ×Ö·û
			  size1  ÏÔÊ¾×Ö·û×ÖºÅ´óÐ¡
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_ShowChar(uint16_t x, uint16_t y, uint16_t chr, uint16_t size1, uint16_t color)
{
	uint16_t i, m, temp, size2, chr1;
	uint16_t x0, y0;
	x0 = x, y0 = y;
	if (size1 == 8)
		size2 = 6;
	else
		size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); // µÃµ½×ÖÌåÒ»¸ö×Ö·û¶ÔÓ¦µãÕó¼¯ËùÕ¼µÄ×Ö½ÚÊý
	chr1 = chr - ' ';											   // ¼ÆËãÆ«ÒÆºóµÄÖµ
	for (i = 0; i < size2; i++)
	{
		if (size1 == 8)
		{
			temp = asc2_0806[chr1][i];
		} // µ÷ÓÃ0806×ÖÌå
		else if (size1 == 12)
		{
			temp = asc2_1206[chr1][i];
		} // µ÷ÓÃ1206×ÖÌå
		else if (size1 == 16)
		{
			temp = asc2_1608[chr1][i];
		} // µ÷ÓÃ1608×ÖÌå
		else if (size1 == 24)
		{
			temp = asc2_2412[chr1][i];
		} // µ÷ÓÃ2412×ÖÌå
		else if (size1 == 48)
		{
			temp = asc2_4824[chr1][i];
		} // µ÷ÓÃ2412×ÖÌå
		else
			return;
		for (m = 0; m < 8; m++)
		{
			if (temp & 0x01)
				Paint_SetPixel(x, y, color);
			else
				Paint_SetPixel(x, y, !color);
			temp >>= 1;
			y++;
		}
		x++;
		if ((size1 != 8) && ((x - x0) == size1 / 2))
		{
			x = x0;
			y0 = y0 + 8;
		}
		y = y0;
	}
}

/*******************************************************************
		º¯ÊýËµÃ÷£ºÏÔÊ¾×Ö·û´®
		½Ó¿ÚËµÃ÷£ºx 		 ×Ö·û´®x×ø±ê²ÎÊý
			  y 		 ×Ö·û´®Y×ø±ê²ÎÊý
							*chr    ÒªÏÔÊ¾µÄ×Ö·û´®
			  size1  ÏÔÊ¾×Ö·û´®×ÖºÅ´óÐ¡
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_ShowString(uint16_t x, uint16_t y, uint8_t *chr, uint16_t size1, uint16_t color)
{
	while (*chr != '\0') // ÅÐ¶ÏÊÇ²»ÊÇ·Ç·¨×Ö·û!
	{
		EPD_ShowChar(x, y, *chr, size1, color);
		chr++;
		x += size1 / 2;
	}
}
/*******************************************************************
		º¯ÊýËµÃ÷£ºÖ¸ÊýÔËËã
		½Ó¿ÚËµÃ÷£ºm µ×Êý
			  n Ö¸Êý
		·µ»ØÖµ£º  mµÄn´Î·½
*******************************************************************/
uint32_t EPD_Pow(uint16_t m, uint16_t n)
{
	uint32_t result = 1;
	while (n--)
	{
		result *= m;
	}
	return result;
}
/*******************************************************************
		º¯ÊýËµÃ÷£ºÏÔÊ¾ÕûÐÍÊý×Ö
		½Ó¿ÚËµÃ÷£ºx 		 Êý×Öx×ø±ê²ÎÊý
			  y 		 Êý×ÖY×ø±ê²ÎÊý
							num    ÒªÏÔÊ¾µÄÊý×Ö
			  len    Êý×ÖµÄÎ»Êý
			  size1  ÏÔÊ¾×Ö·û´®×ÖºÅ´óÐ¡
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/
void EPD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint16_t len, uint16_t size1, uint16_t color)
{
	uint8_t t, temp, m = 0;
	if (size1 == 8)
		m = 2;
	for (t = 0; t < len; t++)
	{
		temp = (num / EPD_Pow(10, len - t - 1)) % 10;
		if (temp == 0)
		{
			EPD_ShowChar(x + (size1 / 2 + m) * t, y, '0', size1, color);
		}
		else
		{
			EPD_ShowChar(x + (size1 / 2 + m) * t, y, temp + '0', size1, color);
		}
	}
}
/*******************************************************************
		º¯ÊýËµÃ÷£ºÏÔÊ¾¸¡µãÐÍÊý×Ö
		½Ó¿ÚËµÃ÷£ºx 		 Êý×Öx×ø±ê²ÎÊý
			  y 		 Êý×ÖY×ø±ê²ÎÊý
							num    ÒªÏÔÊ¾µÄ¸¡µãÊý
			  len    Êý×ÖµÄÎ»Êý
			  pre    ¸¡µãÊýµÄ¾«¶È
			  size1  ÏÔÊ¾×Ö·û´®×ÖºÅ´óÐ¡
			  Color  ÏñËØµãÑÕÉ«²ÎÊý
		·µ»ØÖµ£º  ÎÞ
*******************************************************************/

void EPD_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len, uint8_t pre, uint8_t sizey, uint8_t color)
{
	uint8_t t, temp, sizex;
	uint16_t num1;
	sizex = sizey / 2;
	num1 = num * EPD_Pow(10, pre);
	for (t = 0; t < len; t++)
	{
		temp = (num1 / EPD_Pow(10, len - t - 1)) % 10;
		if (t == (len - pre))
		{
			EPD_ShowChar(x + (len - pre) * sizex, y, '.', sizey, color);
			t++;
			len += 1;
		}
		EPD_ShowChar(x + t * sizex, y, temp + 48, sizey, color);
	}
}

// GUIÏÔÊ¾Ãë±í
void EPD_ShowWatch(uint16_t x, uint16_t y, float num, uint8_t len, uint8_t pre, uint8_t sizey, uint8_t color)
{
	uint8_t t, temp, sizex;
	uint16_t num1;
	sizex = sizey / 2;
	num1 = num * EPD_Pow(10, pre);
	for (t = 0; t < len; t++)
	{
		temp = (num1 / EPD_Pow(10, len - t - 1)) % 10;
		if (t == (len - pre))
		{
			EPD_ShowChar(x + (len - pre) * sizex + (sizex / 2 - 2), y - 6, ':', sizey, color);
			t++;
			len += 1;
		}
		EPD_ShowChar(x + t * sizex, y, temp + 48, sizey, color);
	}
}

// 用这个，勾选颜色反转
void EPD_ShowPicture(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t BMP[], uint16_t Color)
{
	uint16_t j = 0, t;
	uint16_t i, temp, x0, TypefaceNum = sizey * (sizex / 8 + ((sizex % 8) ? 1 : 0));
	x0 = x;
	for (i = 0; i < TypefaceNum; i++)
	{
		temp = BMP[j];
		for (t = 0; t < 8; t++)
		{
			if (temp & 0x80)
			{
				Paint_SetPixel(x, y, !Color);
			}
			else
			{
				Paint_SetPixel(x, y, Color);
			}
			x++;
			temp <<= 1;
		}
		if ((x - x0) == sizex)
		{
			x = x0;
			y++;
		}
		j++;
	}
}
