#include <string>
//#include <memory>
#include <map>
#include <vector>

namespace json
{
	enum json_value_type
	{
		jvt_null,
		jvt_bool,
		jvt_int,
		jvt_double,
		jvt_string, 
		jvt_array,
		jvt_object
	};

	const wchar_t hex_number_map[] =  //from 0 to 'f'
	{
		/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
		/* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

		/* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		/* 6 */ -1,10,11,12, 13,14,15
	};

	class json_value;
	typedef std::vector<json_value> json_array;
	typedef std::map<std::wstring,json_value> json_object;

	bool is_json_white_space(wchar_t c);
	size_t jump_over_white_spaces(const std::wstring& data, size_t& start);
	bool is_digit(wchar_t c);
	std::wstring escape_json(const std::wstring& data);
	std::wstring unescape_json(const std::wstring& data, size_t& start, bool until_dquote);
	std::wstring unescape_json(const std::wstring& data);
	json_value parse_value(const std::wstring& data, size_t& start);
	json_value parse_number(const std::wstring& data, size_t& start);
	json_value parse_string(const std::wstring& data, size_t& start);
	json_value parse_array(const std::wstring& data, size_t& start);
	json_value parse_object(const std::wstring& data, size_t& start);

	class json_value
	{
	public:
		json_value()
		{
			set_value_as_null();
		}

		json_value(bool v)
		{
			set_value_as_bool(v);
		}

		json_value(__int64 v)
		{
			set_value_as_int(v);
		}

		json_value(double v)
		{
			set_value_as_double(v);
		}

		json_value(std::wstring v)
		{
			set_value_as_string(v);
		}

		json_value(json_array v)
		{
			set_value_as_array(v);
		}

		json_value(json_object v)
		{
			set_value_as_object(v);
		}

	public:
		json_value& operator=(void* v)
		{
			set_value_as_null();
			return *this;
		}

		json_value& operator=(bool v)
		{
			set_value_as_bool(v);
			return *this;
		}

		json_value& operator=(__int64 v)
		{
			set_value_as_int(v);
			return *this;
		}

		json_value& operator=(double v)
		{
			set_value_as_double(v);
			return *this;
		}

		json_value& operator=(std::wstring v)
		{
			set_value_as_string(v);
			return *this;
		}

		json_value& operator=(json_array v)
		{
			set_value_as_array(v);
			return *this;
		}

		json_value& operator=(json_object v)
		{
			set_value_as_object(v);
			return *this;
		}

		json_value& operator=(json_value v)
		{
			switch (v.get_value_type())
			{
			case jvt_null:
				set_value_as_null();
				break;
			case jvt_bool:
				set_value_as_bool(v.get_value_as_bool());
				break;
			case jvt_int:
				set_value_as_int(v.get_value_as_int());
				break;
			case jvt_double:
				set_value_as_double(v.get_value_as_double());
				break;
			case jvt_string:
				set_value_as_string(v.get_value_as_string());
				break; 
			case jvt_array:
				set_value_as_array(v.get_value_as_array());
				break;
			case jvt_object:
				set_value_as_object(v.get_value_as_object());
				break;
			default:
				set_value_as_null();
				break;
			}
			return *this;
		}

	public:
		bool is_null()
		{
			return type_ == jvt_null;
		}

		bool get_value_as_bool()
		{
			if (type_ == jvt_bool)
			{
				return jbool_;
			}
			else 
			{
				return false;
			}
		}

		__int64 get_value_as_int()
		{
			if (type_ == jvt_int)
			{
				return jint_;
			}
			else 
			{
				return 0;
			}
		}

		double get_value_as_double()
		{
			if (type_ == jvt_double)
			{
				return jdouble_;
			}
			else 
			{
				return 0.0;
			}
		}

		std::wstring get_value_as_string()
		{
			if (type_ == jvt_string)
			{
				return jstring_;
			}
			else 
			{
				return std::wstring(L"");
			}
		}

		json_array get_value_as_array()
		{
			if (type_ == jvt_array)
			{
				return jarray_;
			}
			else 
			{
				return json_array();
			}
		}

		json_object get_value_as_object()
		{
			if (type_ == jvt_object)
			{
				return jobject_;
			}
			else 
			{
				return json_object();
			}
		}

	public:
		void set_value_as_null()
		{
			type_ = jvt_null;
		}

		void set_value_as_bool(bool v)
		{
			type_ = jvt_bool;
			jbool_ = v;
		}

		void set_value_as_int(__int64 v)
		{
			type_ = jvt_int;
			jint_ = v;
		}

		void set_value_as_double(double v)
		{
			type_ = jvt_double;
			jdouble_ = v;
		}	

		void set_value_as_string(std::wstring v)
		{
			type_ = jvt_string;
			jstring_ = v;
		}

		void set_value_as_array(json_array& v)
		{
			type_ = jvt_array;
			jarray_ = v;
		}

		void set_value_as_object(json_object& v)
		{
			type_ = jvt_object;
			jobject_ = v;
		}

		json_value_type get_value_type()
		{
			return type_;
		}

	public:
		std::wstring to_string()
		{
			std::wstring segment;
			switch (get_value_type())
			{
			case jvt_null:
				segment = L"null";
				break;
			case jvt_bool:
				segment = get_value_as_bool()?L"true":L"false";
				break;
			case jvt_int:
				segment = std::to_wstring(get_value_as_int());
				break;
			case jvt_double:
				segment = std::to_wstring(get_value_as_double());
				break;
			case jvt_string:
				segment += L"\"";
				segment += escape_json(get_value_as_string());
				segment += L"\"";
				break; 
			case jvt_array:
				{
					segment += L"[";
					json_array arr = get_value_as_array();
					for (UINT i = 0; i<arr.size(); i++)
					{
						segment += arr[i].to_string();
						if(i<arr.size()-1)
						{
							segment += L",";
						}
					}
					segment += L"]";
				}
				break;
			case jvt_object:
				{
					segment+=L"{";
					json_object obj = get_value_as_object();
					for (auto it = obj.begin(); it!=obj.end(); it++)
					{
						segment += L"\"";
						segment += escape_json(it->first);
						segment += L"\":";

						segment += it->second.to_string();
						auto it2 = it;
						it2++;
						if (it2!=obj.end())
						{
							segment += L",";
						}
					}
					segment += L"}";
				}
				break;
			default:
				segment = L"null";
				break;
			}

			return segment;
		}

		static json_value from_string(const std::wstring& data)
		{
			size_t start = 0;
			json_value v = parse_value(data, start);
			start = jump_over_white_spaces(data, start);
			return v;
		}

	private:
		json_value_type type_;
		bool jbool_;
		__int64 jint_;
		double jdouble_;
		std::wstring jstring_;
		json_array jarray_;
		json_object jobject_;
	};

	inline bool is_json_white_space(wchar_t c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}

	inline size_t jump_over_white_spaces(const std::wstring& data, size_t& start)
	{
		size_t length = data.length();
		while (start < length && is_json_white_space(data[start])) //jump all white space
		{
			start++;
		}
		return start;
	}

	inline bool is_digit(wchar_t c)
	{
		return c >= '0' && c <= '9';
	}

	inline std::wstring escape_json(const std::wstring& data)
	{
		std::wstring buffer;
		buffer.reserve((size_t)(data.size() * 1.1));
		for (size_t i = 0; i < data.length(); ++i)
		{
			wchar_t c = data[i];
			switch (c)
			{
			case '\\':
				buffer += '\\';
				buffer += '\\';
				break;
			case '\"':
				buffer += '\\';
				buffer += '\"';
				break;
			case '/':
				buffer += '\\';
				buffer += '/';
				break;
			case '\b':
				buffer += '\\';
				buffer += 'b';
				break;
			case '\f':
				buffer += '\\';
				buffer += 'f';
				break;
			case '\n':
				buffer += '\\';
				buffer += 'n';
				break;
			case '\r':
				buffer += '\\';
				buffer += 'r';
				break;
			case '\t':
				buffer += '\\';
				buffer += 't';
				break;
			default:
				buffer += c;
				break;
			}

		}
		return buffer;
	}

	inline std::wstring unescape_json(const std::wstring& data, size_t& start, bool until_dquote)
	{
		std::wstring buffer;
		buffer.reserve(data.length() - start);
		bool found_end_dquote = false;
		while (start < data.length() )
		{
			if (until_dquote && data[start]  == '\"')
			{
				found_end_dquote = true;
				start++;
				break;
			}
			if (data[start] == '\\')        //escape:
			{
				start++;
				switch(data[start])
				{
				case '\"':
					buffer += ('\"');
					start++;
					break;
				case '\\':
					buffer += '\\';
					start++;
					break;
				case '/':
					buffer += '/';
					start++;
					break;
				case 'b':  
					buffer += '\b'; 
					start++; 
					break;
				case 'f':   
					buffer += '\f'; 
					start++;
					break;
				case 'n':   
					buffer += '\n'; 
					start++;
					break;
				case 'r':   
					buffer += '\r'; 
					start++; 
					break;
				case 't':   
					buffer += '\t'; 
					start++; 
					break;
				case 'u':
					{
						wchar_t v = (wchar_t)(hex_number_map[data[start + 1]]<<12
							+ (hex_number_map[data[start + 2]]<<8)
							+ (hex_number_map[data[start + 3]]<<4)
							+ (hex_number_map[data[start + 4]]));
						buffer += v;
						start += 5;
						break;
					}
				default:
					break;
				}
			}
			else
			{
				buffer += data[start];
				start++;
			}
		}
		return buffer;
	}

	inline std::wstring unescape_json(const std::wstring& data)
	{
		size_t start = 0;
		return unescape_json(data, start, false);
	}

	inline json_value parse_value(const std::wstring& data, size_t& start)
	{
		start = jump_over_white_spaces(data, start);
		switch(data[start])
		{
		case 't':
			{
				start += 4;
				return json_value(true);
			}
		case 'f':
			{
				start += 5;
				return json_value(false);
			}
		case 'n':
			{
				start += 4;
				return json_value();
			}
		case '[':
			{
				return parse_array(data, start);
			}
		case '{':
			{
				return parse_object(data, start);
			}
		case '\"':
			{
				return parse_string(data, start);
			}
		default:
			{
				return parse_number(data, start);
			}
		}
	}

	inline json_value parse_number(const std::wstring& data, size_t& start)
	{
		json_value_type type = jvt_int;
		start = jump_over_white_spaces(data, start);
		size_t end = start;
		//check if - exists
		if (data[end] == '-')
		{
			end++; 
		}
		//part before expo and dot
		if (data[end] == '0')
		{
			end++;
		}
		else
		{
			end++;
			while (end< data.length() && is_digit(data[end]))
			{
				end++;
			}
		}
		if (end < data.length())
		{
			//fractional part
			if (data[end] == '.')
			{
				type = jvt_double;
				end++;
				while (end < data.length() && is_digit(data[end]))
				{
					end++;
				}
			}
			//expo part
			if (end < data.length() && (data[end] == 'e' || data[end] == 'E'))
			{
				type = jvt_double;
				end++;
				if (data[end] == '+' || data[end] == '-')
				{
					end++;
				}
				end++;
				while (end < data.length() && is_digit(data[end]))
				{
					end++;
				}

			}
		}
		json_value value;
		if (type == jvt_int)
		{
			wchar_t* endptr;
			__int64 d = _wcstoi64(data.substr(start, end - start).c_str(), &endptr, 10);
			value.set_value_as_int(d);
		}
		else
		{
			wchar_t* endptr = nullptr;
			double d = wcstod(data.substr(start, end - start).c_str(), &endptr);
			value.set_value_as_double(d);
		}
		start = end;
		return value;
	}

	inline json_value parse_string(const std::wstring& data, size_t& start)
	{
		start = jump_over_white_spaces(data, start);
		start++;

		return json_value(unescape_json(data, start, true));
	}

	inline json_value parse_array(const std::wstring& data, size_t& start)
	{
		json_array arr;
		start = jump_over_white_spaces(data, start);
		start++;
		start = jump_over_white_spaces(data, start);
		if (data[start] == ']')
		{
			start++;
			return json_value(std::move(arr));
		}
		else
		{
			arr.push_back(parse_value(data, start));
		}
		start = jump_over_white_spaces(data, start);
		while (start < data.length() && data[start] == ',')
		{
			start++;
			arr.push_back(parse_value(data, start));
			start = jump_over_white_spaces(data, start);
		}
		start++;
		return json_value(std::move(arr));
	}

	inline json_value parse_object(const std::wstring& data, size_t& start)
	{
		json_object obj;
		start = jump_over_white_spaces(data, start);
		start++;
		start = jump_over_white_spaces(data, start);
		if (data[start] == '}')
		{
			start++;
			return json_value(std::move(obj));
		}
		else
		{
			auto key = parse_string(data, start);
			start = jump_over_white_spaces(data, start);
			start++;
			auto value = parse_value(data, start);
			obj[key.get_value_as_string()] = std::move(value); 
		}
		start = jump_over_white_spaces(data, start);
		while (start < data.length() && data[start] == ',')
		{
			start++;
			auto key = parse_string(data, start);
			start = jump_over_white_spaces(data, start);
			start++;
			auto value = parse_value(data, start);
			obj[key.get_value_as_string()] = std::move(value); 
			start = jump_over_white_spaces(data, start);
		}
		start++;
		return json_value(std::move(obj));
	}

}