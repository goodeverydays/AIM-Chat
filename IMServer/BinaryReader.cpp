#include "BinaryReader.h"
namespace BR {
	template<>//特例
	bool BinaryReader::ReadData(string& data)
	{
		const char* pcur = m_buffer.c_str() + m_index;//获取当前位置
		int length = 0;
		size_t i = 0;
		for (; i < m_buffer.size() - m_index; i++)
		{
			length <<= 7;//每次循环将length左移7位，为了将之前读取的长度值向左移动，为新的长度值腾出空间
			length |= pcur[i] & 0x7F;//将当前字节的低7位与length进行按位或运算，得到当前字节表示的长度值，并将其累加到length中
			if ((pcur[i] & 0x80) == 0) break;
		}
		m_index += i + 1;//更新当前读取位置的索引，假设长度参数占用i+1个字节
		data.assign(m_buffer.c_str() + m_index, length);
		m_index += length;//更新当前读取位置的索引，假设字符串对象的大小等于要读取的数据长度
		return true;
	}
	
	template<>//特例
	bool BinaryWriter::WriteData(const string& data)
	{
		string out;
		Compress(data.size(), out);
		m_buffer.append(out.c_str(), out.size());
		m_index += out.size();
		if (data.size() > 0) {
			m_buffer.append(data.c_str(), data.size());
			m_index += data.size();
		}
		return true;
	}
}