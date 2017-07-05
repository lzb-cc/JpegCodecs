
#include "JpegDecoder.h"
#include "Config.h"
#include <map>
#include <string>
#include <stdio.h>
#include <cmath>

using namespace JpegCodec;
using namespace std;

//------------------  Public Field   -----------------------------------------------------//

JpegDecoder::JpegDecoder(const char *fileName) : yDC(0), cbDC(0), crDC(0), endOfDecoder(false), readCount(0x80)
{
    FILE *fp = fopen(fileName, "rb+"); //���ļ�
    /* �����ļ���С */
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    /* �ض�λ���ļ�ͷ */
    fseek(fp, 0, SEEK_SET);

    stream = new uint8_t[length];
    fread(stream, 1, length, fp); // ��ȡ�����ļ�����

    fclose(fp); // �ر��ļ�
}


JpegDecoder::~JpegDecoder()
{
    delete []stream; //free �ļ�������
}


void JpegDecoder::Decoder(Matrix &mat)
{
    /* Preparation */
    ReadQuantTable();
    ReadImageSize();
    ComputeDHT();

    /* ����Matrix */
    mat.Create(m_height, m_width, 3);

    int rowBlkNr = (m_height + 15) >> 4; // ��ֱ����ֿ�
    int colBlkNr = (m_width + 15) >> 4;  // ˮƽ����ֿ�

    /* ���� */
	ToStartOfData();
    for (int i = 0; i < rowBlkNr; i++)
    {
        for (int j = 0; j < colBlkNr; j++)
        {
			if (endOfDecoder) return;

            DecoderMCU(); // ����һ�� MacroBlock
            FillYCbCr();  // ��� YCbCr ������

            /* д�뵽 mat */
            ConvertClrSpace(mat.data,i * 16, j * 16);
        }
    }
}

//------------------  Proteced Field   ----------------------------------------------------//

//------------------  0.helper function   ----------------------------------

/* @brief ��ȡѹ�����ݵ���һ����Чλ
 */
int JpegDecoder::NextBit()
{
    if (readCount == 0x0)
    {
        // reset
        readCount = 0x80;
        curIndex ++;

        // check
        if (stream[ curIndex ] == 0xFF) //���ֵ
        {
            curIndex ++;
            if (stream[ curIndex ] & 0xD7)       ResetDC();
            else if (stream[ curIndex ] == 0xD9) endOfDecoder = true;
            else if (stream[ curIndex ] == 0x00) stream[ curIndex ] = 0xFF;
        }
    }

    int retVal = stream[ curIndex ] & readCount; // ��ȡ��ǰλ��ֵ (1 or 0)
    readCount >>= 1;
    return (retVal > 0 ? 1 : 0);
}


/* @brief �����׼�����������ʵֵ
 */
int JpegDecoder::ComputeRealValue(int length)
{
    int retVal = 0;
    for (int i = 0; i < length; i++)
    {
        retVal = (retVal << 1) + NextBit();
    }

    return (retVal >= pow(2, length - 1) ? retVal : retVal - pow(2, length) + 1);
}

/* @brief ������
 */
void JpegDecoder::UpSample(int32_ptr CbCr, int32_ptr blk)
{
    typedef int (*Ptr_CbCr)[16];
    Ptr_CbCr p = reinterpret_cast<Ptr_CbCr>(CbCr); // һλ����ת��ά����

    for (int i = 0; i < 16; i++)
    {
        for (int j =0; j< 16; j++)
        {
            p[i][j] = blk[(i >> 1) * 8 + (j >> 1)];
        }
    }


}

/* @brief Reset DC ֵ
 */
void JpegDecoder::ResetDC()
{
    yDC = cbDC = crDC = 0x0;
}


/* @brief ���Ҷ�Ӧ�ı�Ƕε�����
* @mark: �α��
*/
int JpegDecoder::MarkIndex(uint8_t mark)
{
    int idx = 0;
    while(stream[idx] != 0xFF || stream[idx + 1] != mark) idx ++;
    return idx;
}


/* @brief �Ӷ������������в���һ����Ч�ı���ֵ
 * @table:    ���ڽ���Ĺ�������
 */
int JpegDecoder::FindKeyValue(std::map<std::string, uint8_t> &table)
{
    string keyStr = "";
    while (table.find(keyStr) == table.end())
    {
        keyStr += (NextBit() + '0'); // char of 0 or 1
    }

    return table[keyStr];
}

/* @brief ��һ�� MacroBlock ��䵽 YCbCr ������
 */
void JpegDecoder::FillYCbCr()
{
    Ptr16 yPtrDst = (Ptr16)yBuf;
    for (int k = 0; k < 4; k++)
    {
        int xOffset = (k >> 1) << 3;  // equals: (k / 2) * 8;
        int yOffset = (k & 0x1) << 3; // equals: (k % 2) * 8;
        Ptr8 yPtrSrc = (Ptr8)yBlk[k];
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                yPtrDst[xOffset + i][yOffset + j] = yPtrSrc[i][j];
            }
        }
    }

    // ������
    UpSample(cbBuf, cbBlk);
    UpSample(crBuf, crBlk);
}

//------------------  1.����׼������   -------------------------------------

/* ��ȡ������ */
void JpegDecoder::ReadQuantTable()
{
    int base = MarkIndex(DQT);

    int baseY = base + 5;
    int baseCbCr = base + 74;
    for (int i = 0; i < 64; i++)
    {
        quantY[i] = stream[i + baseY];
        quantCbCr[i] = stream[i + baseCbCr];
    }
}

/* ��ȡͼ��Ŀ�� */
void JpegDecoder::ReadImageSize()
{
    int base = MarkIndex(SOF);

    m_height = (stream[base + 5] << 8) + stream[base + 6];
    m_width  = (stream[base + 7] << 8) + stream[base + 8];
}

/* @brief ������������
 * @base:  ��DHT_Segment �ı�ͷ��ʼ�����������ֶκ�һ���ֽڵ�ƫ����
 * @table: Ҫ�����Ĺ�������
 */
void JpegDecoder::ReBuildTable(int base, map<string, uint8_t> &table)
{
    int offset = 16;  // offset
    string keyStr = "";
    for (int i = 0; i < 16; i++) // length of key (i.e. i = 2 means key = 000 , 001 , 010 , 011 or ...)
    {
        int cnt = DHT_Segment[base + i]; // number of key, which length is (i+1)

        /* alignment */
        for (int k = keyStr.length(); k <= i; k++)
        {
            keyStr += "0";
        }

        while (cnt > 0)
        {
            /* value of key */
            table.insert(pair<string, uint8_t>(keyStr, DHT_Segment[base + offset]));
            offset++;

            /* increment */
            int carry = 1; //��λ
            for (int k = keyStr.length() - 1; k >= 0; k--)
            {
                int tmpVal = (keyStr[k] + carry - '0'); //�����λ
                carry = tmpVal / 2;
                keyStr[k] = tmpVal % 2 + '0'; //�����ǰλ���
            }
            cnt = cnt - 1;
        }
    }
}

/* ���� DC AC �������� */
void JpegDecoder::ComputeDHT()
{
    ReBuildTable(5, DC[0]);
    ReBuildTable(34, DC[1]);
    ReBuildTable(63, AC[0]);
    ReBuildTable(242, AC[1]);
}

/* ��λ������ͷ */
void JpegDecoder::ToStartOfData()
{
    curIndex = MarkIndex(SOS) + 14;
}

//------------------  2.����һ��block   ------------------------------------

/* @brief ������һ�� 8 x 8 �����ݿ�
 * @out:     ����ֵ��һ�� 8 x 8 �����ݿ�
 * @dcTable: DC��������
 * @actable: AC��������
 * @dc:      ǰһ��block�� DC ֵ
 */
void JpegDecoder::DecoderBlock(int32_ptr out, map<string, uint8_t> &dcTable, map<string, uint8_t> &acTable, int &dc)
{
    // reset matrix
    for (int i = 0; i < 64; i++) out[i] = 0x0;

    // decoder DC of matrix
    int length = FindKeyValue(dcTable);
    int value = ComputeRealValue(length);
    dc += value; // DC
    out[0] = dc;

    // decoder AC of matrix
    for (int i = 1; i < 64; i++)
    {
        length = FindKeyValue(acTable);
        if (length == 0x0) break; // ��������

        value = ComputeRealValue(length & 0xf); // �ұ� 4λ��ʵ��ֵ����
        i += (length >> 4);          // ��� 4λ���г̳���
        out[i] = value; // AC
    }
}


//------------------  3.������   -------------------------------------------

/* @brief ������
 * @out:    ���������ݣ�����ֵ��8x8��
 * @quant:  ������8x8��
 */
void JpegDecoder::Dequant(int32_ptr out, int8_ptr quant)
{
    for (int i = 0; i < 64; i++)
    {
        out[i] = quant[i] * out[i];
    }
}

//------------------  4.�� ZigZag ����   -----------------------------------

/* @brief �� ZigZag ����
 * @out:  ����ֵ��8x8��
 * @out:  ԭʼ���ݣ�8x8��
 */
void JpegDecoder::UnZigZag(int32_ptr out, int32_ptr source)
{
    for (int i = 0; i < 64; i++) out[i] = source[UnZigZagTable[i]];
}

//------------------  5.����ɢ���ұ任��IDCT��   ---------------------------

/* @brief ����ɢ���ұ任
 * @out:    ����ֵ�� 8 x 8 block
 */
void JpegDecoder::Transform(int32_ptr out)
{
    double tmp[8][8];
    Ptr8 pIn = (Ptr8)out; // �ö�ά���鷽ʽ����һλ����
    double tmpVal;

    /* tmp = MtxIDCT * Matrix */
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            tmpVal = 0;
            for (int k = 0; k < 8; k++)
            {
                tmpVal += MtxIDCT[i][k] * pIn[k][j];
            }
            tmp[i][j] = round(tmpVal);
        }
    }

    /* Matrix = tmp * MtxDCT */
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            tmpVal =0;
            for (int k = 0; k < 8; k++)
            {
                tmpVal +=tmp[i][k] * MtxDCT[k][j];
            }
            pIn[i][j] = round(tmpVal);
        }
    }
}

//------------------  6.����һ�� MacroBlock   ------------------------------

/* ����һ�� MacroBlock */
void JpegDecoder::DecoderMCU()
{
    int tmp[64];

    /* 4 �� Y ���� */
    for (int i = 0; i < 4; i++)
    {
        DecoderBlock(tmp, DC[0], AC[0], yDC); // ����һ�����ݿ�
        Dequant(tmp, quantY);                         // ������
        UnZigZag(yBlk[i], tmp);                        // �� ZigZag ����
        Transform(yBlk[i]);                       // ����ɢ���ұ任 ��IDCT��
    }
    /* һ�� Cb ���� */
    DecoderBlock(tmp, DC[1], AC[1], cbDC);
    Dequant(tmp, quantCbCr);
    UnZigZag(cbBlk, tmp);
    Transform(cbBlk);

    /* һ�� Cr ���� */
    DecoderBlock(tmp, DC[1], AC[1], crDC);
    Dequant(tmp, quantCbCr);
    UnZigZag(crBlk, tmp);
    Transform(crBlk);
}

//------------------  7.��ɫ�ռ�ת����from YCbCr to RGB��   ----------------

/* ��ɫ�ռ�ת��
 * @outBgr:  16x16x3 block ,����ֵ��BGR��ʽ��������
 * @row:  ��ǰ���ݿ�� row index , ����Խ����
 * @col:  ��ǰ���ݿ�� col index�� ����Խ����
 */
void JpegDecoder::ConvertClrSpace(int8_ptr outBgr, int row, int col)
{
    Ptr16 Y  = (Ptr16) yBuf;
    Ptr16 Cb = (Ptr16) cbBuf;
    Ptr16 Cr = (Ptr16) crBuf;
	int R, G, B;

	outBgr += (row * m_width + col) * 3; // ƫ�Ƶ� �� (x,y) ��
    int stride = m_width * 3; // һ��ƫ��һ��
    for (int i = 0; i < 16; i++)
    {
		if (row + i >= m_height) break; // Խ����
        // ����һ��
        for (int j = 0; j < 16; j++)
        {
            if (col + j >= m_width) break; // Խ����
            int offset = j * 3;
            R = Y[i][j] + 1.402 * Cr[i][j] + 128;                         // R
            G = Y[i][j] - 0.34414 * Cb[i][j] - 0.71414 * Cr[i][j] + 128;  // G
            B = Y[i][j] + 1.772 * Cb[i][j] + 128;                          // B

			if (R > 255) R = 255;
			if (G > 255) G = 255;
			if (B > 255) B = 255;

			if (R < 0) R = 0;
			if (G < 0) G = 0;
			if (B < 0) B = 0;

			outBgr[offset + 0] = B;
			outBgr[offset + 1] = G;
			outBgr[offset + 2] = R;
        } // end of for_j

        outBgr += stride; // ƫ�Ƶ���һ����
    } // end of for_i
}
