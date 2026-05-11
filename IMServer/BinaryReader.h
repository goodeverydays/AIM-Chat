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

	template<>//льюЩ
	bool BinaryReader::ReadData(string& data)
	{
		//TODO:
		return true;
	}
}