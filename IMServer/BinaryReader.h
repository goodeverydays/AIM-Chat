#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <cstdint>

using namespace std;
namespace BR {

	class BinaryReader
	{
	public:
		static void dump(const string& buf)
		{
			dump(buf.c_str(), buf.size());
		}
		static void dump(const char* buf, size_t size)//二进制数据的十六进制输出函数，接受一个字符指针和一个大小参数，用于将二进制数据以十六进制格式输出到控制台，假设输入的二进制数据存储在buf指向的内存区域中，大小为size字节
		{
			for (size_t i = 0; i < size; i++)
			{
				if (i != 0 && ((i % 16) == 0)) printf("\r\r");
				printf("%02X ", (unsigned)(buf[i]) & 0xFF);
			}
			printf("\r\n");
		}
	public:
		BinaryReader()
		{
			m_index = 0;
		}
		BinaryReader(const BinaryReader& reader)
		{
			m_buffer = reader.m_buffer;
			m_index = reader.m_index;
		}
		BinaryReader(const string& buffer)
		{
			m_index = 0;
			m_buffer = buffer;

		}
		~BinaryReader() = default;
		BinaryReader& operator=(const BinaryReader& reader) {
			if (this != &reader)
			{
				m_buffer = reader.m_buffer;
				m_index = reader.m_index;
			}
			return *this;
		}
		void UpdateBuffer(const string& buffer)
		{
			m_buffer = buffer;
			m_index = 0;
		}
		void Reset() { m_index = 0; }
		size_t Size() { return m_buffer.size(); }
		bool ReadInt32(int32_t& data)
		{
			if (m_index + sizeof(int32_t) > m_buffer.size()) return false;
			memcpy(&data, m_buffer.c_str() + m_index, sizeof(int32_t));
			m_index += sizeof(int32_t);
			return true;
		}
		template<class T>
		bool ReadData(T& data)
		{
			if (m_index + sizeof(T) > m_buffer.size()) return false;
			char* pData = (char*)&data;
			for (size_t i = 0; i < sizeof(T); i++)
			{
				pData[i] = *(m_buffer.c_str() + m_index + sizeof(T) - (i + 1));
			}
			m_index += sizeof(T);
			return true;
		}
		
	private:
		string m_buffer;
		uint32_t m_index;
	};
	
	template<>//特例
	bool BinaryReader::ReadData(string& data);
	

	class BinaryWriter//二进制写入器类，提供将数据写入缓冲区的功能
	{
	public:
		BinaryWriter() : m_index(0)
		{
		}
		~BinaryWriter() = default;
		template<class T>
		bool WriteData(const T& data)
		{
			m_buffer.resize(m_index + sizeof(T));//调整缓冲区的大小以适应要写入的数据，假设数据类型T的大小等于要写入的数据长度
			memcpy((char*)m_buffer.c_str()+m_index, &data, sizeof(data));//将数据复制到缓冲区中，假设数据类型T的大小等于要写入的数据长度
			m_index += sizeof(T);//更新当前写入位置的索引，假设数据类型T的大小等于要写入的数据长度
			return true;
		}
		uint32_t Size() { return m_index; }
		string toString()const { return m_buffer; }
		string toSendString()const
		{
			string out = m_buffer;
			BinaryWriter writer;
			int len = (int)out.size();
			writer.WriteData(len + 6);
			writer.WriteData(htonl(len));
			writer.WriteData(htonl(0));
			out = writer.toString() + out;
			return out;
		}
		void Clear() { m_buffer.clear(); m_index = 0; }

	protected:
		void Compress(size_t len, string& out) //压缩算法函数，接受一个长度参数和一个输出字符串参数，用于将数据进行压缩并存储在输出字符串中，假设压缩算法已经实现，并且可以根据长度参数对数据进行压缩
		{
			char c = 0;
			size_t outlen = 0;
			if(len < 128)
			{
				c = (char)len & 0x7F;//
				out += c;//将长度参数转换为一个字节并追加到输出字符串中，假设长度参数小于128，可以直接使用一个字节来表示长度
				return;
			}
			//假定这个长度不会太长百兆以内,输出的时候不会超过5个字节，32个bit能够表达
			for (int i = 4; i >= 0; i--)
			{
				c = (len >> (7 * i)) & 0x7F;
				if (c == 0 && out.size() == 0)//目前还没有发现有效数据，都是0
				{
					continue;
				}
				if (i > 0)//说明已经是最后7位了
					c |= 0x80;
				out += c;
			}
		}

	private:
		string m_buffer;
		uint32_t m_index;
	};
	template<>//特例
	bool BinaryWriter::WriteData(const string& data);
}