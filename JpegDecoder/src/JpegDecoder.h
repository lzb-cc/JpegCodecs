
#ifndef JPEGDECODER__H
#define JPEGDECODER__H

#include "Config.h"
#include "Stl.h"
#include <string>

namespace JpegCodec
{
	class Matrix
	{
	public:
		Matrix(){}
		void Create(int _rows, int _cols, int _channal)
		{
		    rows = _rows;
		    cols = _cols;
		    channal = _channal;
		    data = new uint8_t[_rows * _cols * _channal];
		}

		int rows;
		int cols;
		int channal;
		uint8_t *data;
	};

	class JpegDecoder
	{
	public:
		JpegDecoder(const char *fileName);
		~JpegDecoder();

		void Decoder(Matrix &mat);
	protected:
        //------------------  0.helper function   ----------------------------------

        /* @brief ��ȡѹ�����ݵ���һ����Чλ
         */
        int NextBit();

        /* @brief �����׼�����������ʵֵ
         */
        int ComputeRealValue(int length);

        /* @brief ������, ͼ��Ŵ�
         * @CbCr:   ����ֵ�� 16 x 16 block
         * @blk:    8x8 block
         */
        void UpSample(int32_ptr CbCr, int32_ptr blk);

        /* @brief Reset DC to 0x0
         */
        void ResetDC();


        /* @brief ���Ҷ�Ӧ�ı�Ƕε�����
         * @mark: �α��
         */
        int MarkIndex(uint8_t mark);

        /* @brief �Ӷ������������в���һ����Ч�ı���ֵ
         * @table:    ���ڽ���Ĺ�������
         */
        int FindKeyValue(tinyStl::tinyMap &table);

        /* @brief ��һ�� MacroBlock ��䵽 YCbCr ������
         */
        void FillYCbCr();

        /* @brief ������������
         * @base:  ��DHT_Segment �ı�ͷ��ʼ�����������ֶκ�һ���ֽڵ�ƫ����
         * @table: Ҫ�����Ĺ�������
         */
        void ReBuildTable(int base, tinyStl::tinyMap &table);

		//------------------  1.����׼������   -------------------------------------

		/* ��ȡ������ */
		void ReadQuantTable();

		/* ��ȡͼ��Ŀ�� */
		void ReadImageSize();

		/* ���� DC AC �������� */
		void ComputeDHT();

		/* ��λ������ͷ */
		void ToStartOfData();


		//------------------  2.����һ��block   ------------------------------------

        /* @brief ������һ�� 8 x 8 �����ݿ�
         * @out:     ����ֵ��һ�� 8 x 8 �����ݿ�
         * @dcTable: DC��������
         * @actable: AC��������
         * @dc:      ǰһ��block�� DC ֵ
         */
        void DecoderBlock(int32_ptr out, tinyStl::tinyMap &dcTable, tinyStl::tinyMap &acTable, int &dc);


		//------------------  3.������   -------------------------------------------

        /* @brief ������
         * @out:    ���������ݣ�����ֵ��8x8��
         * @quant:  ������8x8��
         */
        void Dequant(int32_ptr out, int8_ptr quant);

		//------------------  4.�� ZigZag ����   -----------------------------------

        /* @brief �� ZigZag ����
         * @out:  ����ֵ��8x8��
         * @out:  ԭʼ���ݣ�8x8��
         */
        void UnZigZag(int32_ptr out, int32_ptr source);

		//------------------  5.����ɢ���ұ任��IDCT��   ---------------------------

        /* @brief ����ɢ���ұ任
         * @out:    ����ֵ�� 8 x 8 block
         */
        void Transform(int32_ptr out);

        //------------------  6.����һ�� MacroBlock   ------------------------------

		/* ����һ�� MacroBlock */
		void DecoderMCU();

		//------------------  7.��ɫ�ռ�ת����from YCbCr to RGB��   ----------------

		/* ��ɫ�ռ�ת��
		 * @outBgr:  16x16x3 block ,����ֵ��BGR��ʽ��������
		 * @row:  ��ǰ���ݿ�� row index , ����Խ����
		 * @col:  ��ǰ���ݿ�� col index�� ����Խ����*/
		void ConvertClrSpace(int8_ptr outBgr, int row, int col);

	private:
		uint8_t       *stream;      //Jpeg�ļ�������

		bool           endOfDecoder; // �������������
		int	       curIndex;     // ���뵽��ǰ����λ
		uint8_t        readCount;    // �Ѿ�����ĵ�ǰ�ֽڵ�λ��

		uint8_t        quantY[64];       // Y ����������
		uint8_t        quantCbCr[64];    // CbCr ����������

		int            m_width;          // ͼ����
		int            m_height;         // ͼ��߶�

		int            yBlk[4][64];
		int            cbBlk[64];
		int            crBlk[64];

		int            yBuf[256], cbBuf[256], crBuf[256];

		int            yDC, cbDC, crDC;

		/* huffman table */
		tinyStl::tinyMap DC[2], AC[2];
	};

}


#endif
