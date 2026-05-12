#include <iostream>
#include <string>

using namespace std;
namespace BR {

	class BinaryReader
	{
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
			memcpy(&data, m_buffer.c_str() + m_index, sizeof(T));
			m_index += sizeof(T);
			return true;
		}
		
	private:
		string m_buffer;
		uint32_t m_index;
	};

	template<>//特例
	bool BinaryReader::ReadData(string& data)
	{
		memcpy((char*)&data, m_buffer.c_str() + m_index, data.size());//将缓冲区中的数据复制到字符串对象中，假设字符串对象已经分配了足够的内存来存储数据
		m_index += data.size();//更新当前读取位置的索引，假设字符串对象的大小等于要读取的数据长度
		return true;
	}

	class BinaryWriter//二进制写入器类，提供将数据写入缓冲区的功能
	{
	public:
		BinaryWriter() : m_index(0)
		{
		}
		~BinaryWriter() = default;
		template<class T>
		bool WriteData(T& data)
		{
			m_buffer.resize(m_index + sizeof(T));//调整缓冲区的大小以适应要写入的数据，假设数据类型T的大小等于要写入的数据长度
			memcpy((char*)m_buffer.c_str()+m_index, &data, sizeof(data));//将数据复制到缓冲区中，假设数据类型T的大小等于要写入的数据长度
			m_index += sizeof(T);//更新当前写入位置的索引，假设数据类型T的大小等于要写入的数据长度
			return true;
		}
		uint32_t Size() { return m_index; }
		string toString()const { return m_buffer; }
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
	bool BinaryWriter::WriteData(string& data)
	{
		string out;
		Compress(data.size(), out);
		m_buffer.append(out.c_str(), out.size());
		m_index += out.size();
		if (data.size() > 0) {
			m_buffer.append(data.c_str(), data.size());
			m_buffer += data.size();
		}
		return true;
	}
}