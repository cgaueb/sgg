#pragma once

namespace math
{
template <typename S=float>
class vec2_traits
	{
	public:
		union { S x, u; };
		union { S y, v; };
		S & operator [] (size_t index)
		{
			return *((S*)this + index);
		}

		vec2_traits<S> operator + (const vec2_traits<S> & right)
		{
			vec2_traits<S> left;
			left.x = x + right.x;
			left.y = y + right.y;
			return left;
		}

		vec2_traits<S> operator - (const vec2_traits<S> & right)
		{
			vec2_traits<S> left;
			left.x = x - right.x;
			left.y = y - right.y;
			return left;
		}
		
		vec2_traits<S> operator * (const vec2_traits<S> & right)
		{
			vec2_traits<S> left;
			left.x = x * right.x;
			left.y = y * right.y;
			return left;
		}

		vec2_traits<S> operator * (S right)
		{
			vec2_traits<S> left;
			left.x = x * right;
			left.y = y * right;
			return left;
		}

		vec2_traits<S> operator / (S right)
		{
			vec2_traits<S> left;
			left.x = x / right;
			left.y = y / right;
			return left;
		}

		vec2_traits<S> operator / (const vec2_traits<S> & right)
		{
			vec2_traits<S> left;
			left.x = x / right.x;
			left.y = y / right.y;
			return left;
		}

		vec2_traits<S> & operator += (const vec2_traits<S> & right)
		{
			x += right.x;
			y += right.y;
			return *this;
		}

		vec2_traits<S> & operator -= (const vec2_traits<S> & right)
		{
			x -= right.x;
			y -= right.y;
			return *this;
		}

		vec2_traits<S> & operator /= (const vec2_traits<S> & right)
		{
			x /= right.x;
			y /= right.y;
			return *this;
		}

		vec2_traits<S> & operator *= (const vec2_traits<S> & right)
		{
			x *= right.x;
			y *= right.y;
			return *this;
		}

		vec2_traits<S> & operator *= (S right)
		{
			x *= right;
			y *= right;
			return *this;
		}
		
		vec2_traits<S> & operator /= (S right)
		{
			x /= right;
			y /= right;
			return *this;
		}

		vec2_traits<S>(S x, S y) : x(x), y(y) {}

		vec2_traits<S>(S val) : x(val), y(val) {}

		vec2_traits<S>() : x(), y() {}

		vec2_traits<S>(const vec2_traits<S> & right) : x(right.x), y(right.y) {}
		
		vec2_traits<S> & operator = (const vec2_traits<S> & right)
		{
			x = right.x;
			y = right.y;
			return *this;
		}

		bool operator == (const vec2_traits<S> & right) const
		{
			return x == right.x && y == right.y;
		}

		bool operator != (const vec2_traits<S> & right) const
		{
			return x != right.x || y != right.y;
		}

		S length() const
		{
			return (S)sqrt(x * x + y * y);
		}
	};

	template<typename S>
	vec2_traits<S> operator * (S a, vec2_traits<S> v)
	{
		return v * a;
	}

	template<typename S>
	vec2_traits<S> operator * (int a, vec2_traits<S> v)
	{
		return v * S(a);
	}

	template<typename S>
	vec2_traits<S> operator * (vec2_traits<S> v, int a)
	{
		return v * S(a);
	}

	template<typename S>
	vec2_traits<S> operator / (vec2_traits<S> v, int a)
	{
		return v / S(a);
	}

	template<typename S>
	vec2_traits<S> normalize(vec2_traits<S> v)
	{
		return v / v.length();
	}

	template<typename S>
	S dot(vec2_traits<S> a, vec2_traits<S> b)
	{
		return a.x * b.x + a.y * b.y;
	}


	template<typename S>
	vec2_traits<S> reflect(vec2_traits<S> v, vec2_traits<S> n)
	{
		return v - 2.0f * dot(v, n) * n;
	}

	typedef vec2_traits<float> vec2;

} // namespace math


