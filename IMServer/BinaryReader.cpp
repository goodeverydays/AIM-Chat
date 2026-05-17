#include "BinaryReader.h"
namespace BR {
	template<>//特例
	bool BinaryReader::ReadData(string& data)
	{
		memcpy((char*)data.c_str(), m_buffer.c_str() + m_index, data.size());//将缓冲区中的数据复制到字符串对象中，假设字符串对象已经分配了足够的内存来存储数据
		m_index += data.size();//更新当前读取位置的索引，假设字符串对象的大小等于要读取的数据长度
		return true;
	}
	
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