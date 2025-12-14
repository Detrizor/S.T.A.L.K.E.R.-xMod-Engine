#pragma once
#include <functional>
#include <oneapi/tbb.h>

using ::std::swap;

#include "_type_traits.h"

#ifdef __BORLANDC__
#define M_NOSTDCONTAINERS_EXT
#endif
#ifdef _M_AMD64
#define M_DONTDEFERCLEAR_EXT
#endif

#define M_DONTDEFERCLEAR_EXT //. for mem-debug only

//--------
#ifdef M_NOSTDCONTAINERS_EXT

#define xr_list ::std::list
#define xr_deque ::std::deque
#define xr_stack ::std::stack
#define xr_set ::std::set
#define xr_multiset ::std::multiset
#define xr_map ::std::map
#define xr_hash_map ::std::hash_map
#define xr_multimap ::std::multimap
#define xr_string ::std::string

template <class T>
class xr_vector : public ::std::vector < T >
{
public:
	typedef size_t size_type;
	typedef T& reference;
	typedef const T& const_reference;
public:
	xr_vector() : ::std::vector<T>() {}
	xr_vector(size_t _count, const T& _value) : ::std::vector<T>(_count, _value) {}
	explicit xr_vector(size_t _count) : ::std::vector<T>(_count) {}
	void clear() { erase(begin(), end()); }
	void clear_and_free() { ::std::vector<T>::clear(); }
	void clear_not_free() { erase(begin(), end()); }
	ICF const_reference operator[] (size_type _Pos) const { {VERIFY(_Pos < size()); } return (*(begin() + _Pos)); }
	ICF reference operator[] (size_type _Pos) { {VERIFY(_Pos < size()); } return (*(begin() + _Pos)); }
};

template <>
class xr_vector<bool> : public ::std::vector < bool >
{
	typedef bool T;
public:
	xr_vector<T>() : ::std::vector<T>() {}
	xr_vector<T>(size_t _count, const T& _value) : ::std::vector<T>(_count, _value) {}
	explicit xr_vector<T>(size_t _count) : ::std::vector<T>(_count) {}
	u32 size() const { return (u32)::std::vector<T>::size(); }
	void clear() { erase(begin(), end()); }
};

#else

template <class T>
class xalloc
{
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

public:
	template<class _Other>
	struct rebind { typedef xalloc<_Other> other; };
public:
	pointer address(reference _Val) const { return (&_Val); }
	const_pointer address(const_reference _Val) const { return (&_Val); }
	xalloc() { }
	xalloc(const xalloc<value_type>&) { }
	template<class _Other> xalloc(const xalloc<_Other>&) { }
	template<class _Other> xalloc<value_type>& operator= (const xalloc<_Other>&) { return (*this); }
	pointer allocate(size_type n, const void* p = 0) const { return xr_alloc<value_type>((u32)n); }
	char* _charalloc(size_type n) { return (char*)allocate(n); }
	void deallocate(pointer p, size_type n) const { xr_free(p); }
	void deallocate(void* p, size_type n) const { xr_free(p); }
	void destroy(pointer p) { p->~T(); }
	size_type max_size() const { size_type _Count = (size_type)(-1) / sizeof(value_type); return (0 < _Count ? _Count : 1); }
	
	template <typename... Args>
	void construct(pointer p, Args&&... args) { new (p)value_type(_STD forward<Args>(args)...); }
};

struct xr_allocator
{
	template <typename T>
	struct helper
	{
		typedef xalloc<T> result;
	};

	static void* alloc(const u32& n) { return xr_malloc((u32)n); }
	template <typename T>
	static void dealloc(T*& p) { xr_free(p); }
};

template<class _Ty, class _Other> inline bool operator==(const xalloc<_Ty>&, const xalloc<_Other>&) { return (true); }
template<class _Ty, class _Other> inline bool operator!=(const xalloc<_Ty>&, const xalloc<_Other>&) { return (false); }

namespace std
{
template<class _Tp1, class _Tp2> inline xalloc<_Tp2>& __stl_alloc_rebind(xalloc<_Tp1>& __a, const _Tp2*) { return (xalloc<_Tp2>&)(__a); }
template<class _Tp1, class _Tp2> inline xalloc<_Tp2> __stl_alloc_create(xalloc<_Tp1>&, const _Tp2*) { return xalloc<_Tp2>(); }
};

// string(char)
typedef ::std::basic_string<char, ::std::char_traits<char>, xalloc<char> > xr_string;

// vector
template <typename T, typename allocator = xalloc<T>>
class xr_vector : public _STD vector <T, allocator>
{
private:
	typedef _STD vector<T, allocator> inherited;

public:
	typedef allocator allocator_type;

public:
	xr_vector() : inherited() {}
	xr_vector(size_t _count, const T& _value) : inherited(_count, _value) {}
	explicit xr_vector(size_t _count) : inherited(_count) {}
	u32 size() const { return (u32)inherited::size(); }

	void clear_and_free() { inherited::clear(); }
	void clear_not_free() { erase(begin(), end()); }
	void clear_and_reserve() { if (capacity() <= (size() + size() / 4)) clear_not_free(); else { u32 old = size(); clear_and_free(); reserve(old); } }

#ifdef M_DONTDEFERCLEAR_EXT
	void clear() { clear_and_free(); }
#else
	void clear() { clear_not_free(); }
#endif

	const_reference operator[] (size_type _Pos) const { {R_ASSERT2(_Pos < size(), make_string("index is out of range: index requested[%d], size of container[%d]", _Pos, size()).c_str()); } return (*(begin() + _Pos)); }
	reference operator[] (size_type _Pos) { {R_ASSERT2(_Pos < size(), make_string("index is out of range: index requested[%d], size of container[%d]", _Pos, size()).c_str()); } return (*(begin() + _Pos)); }

	template <typename... Args>
	T& emplace_back(Args&&... args) { inherited::emplace_back(_STD forward<Args>(args)...); return back(); }

	const_iterator find(const T& obj) const
	{
		return _STD find(inherited::begin(), inherited::end(), obj);
	}

	iterator find(T& obj)
	{
		return _STD find(inherited::begin(), inherited::end(), obj);
	}

	bool contains(const T& obj) const
	{
		return find(obj) != inherited::end();
	}

	bool contains(T& obj)
	{
		return find(obj) != inherited::end();
	}

	bool erase_checked(iterator it, bool assert = true)
	{
		if (it != inherited::end())
		{
			erase(it);
			return true;
		}
		else if (assert)
			FATAL("data not found in vector");
		return false;
	}

	bool erase_data(const T& obj, bool assert = true)
	{
		return erase_checked(_STD find(inherited::begin(), inherited::end(), obj), assert);
	}

	template <typename Eval>
	const_iterator find_if(const Eval& obj) const
	{
		return _STD find_if(inherited::begin(), inherited::end(), obj);
	}

	template <typename Eval>
	iterator find_if(const Eval& obj)
	{
		return _STD find_if(inherited::begin(), inherited::end(), obj);
	}

	template <typename Eval>
	bool contains_if(const Eval& obj) const
	{
		return find_if(obj) != inherited::end();
	}

	template <typename Eval>
	bool contains_if(Eval& obj)
	{
		return find_if(obj) != inherited::end();
	}

	template <typename Eval>
	bool erase_data_if(const Eval& obj, bool assert = true)
	{
		return erase_checked(_STD find_if(inherited::begin(), inherited::end(), obj), assert);
	}

	template <typename _Pr>
	IC void sort(_Pr _Pred)
	{
		tbb::parallel_sort(inherited::begin(), inherited::end(), _Pred);
	}

	IC void sort()
	{
		tbb::parallel_sort(inherited::begin(), inherited::end());
	}
};

// vector<bool>
template <>
class xr_vector<bool, xalloc<bool> > : public ::std::vector < bool, xalloc<bool> >
{
private:
	typedef ::std::vector<bool, xalloc<bool> > inherited;

public:
	u32 size() const { return (u32)inherited::size(); }
	void clear() { erase(begin(), end()); }
};

template <typename allocator>
class xr_vector<bool, allocator> : public ::std::vector < bool, allocator >
{
private:
	typedef ::std::vector<bool, allocator> inherited;

public:
	u32 size() const { return (u32)inherited::size(); }
	void clear() { erase(begin(), end()); }
};

// deque
template <typename T, typename allocator = xalloc<T> >
class xr_deque : public ::std::deque < T, allocator >
{
public:
	typedef typename allocator allocator_type;
	typedef typename allocator_type::value_type value_type;
	typedef typename allocator_type::size_type size_type;
	u32 size() const { return (u32)__super::size(); }
};

// stack
template <typename _Ty, class _C = xr_vector<_Ty> >
class xr_stack
{
public:
	typedef typename _C::allocator_type allocator_type;
	typedef typename allocator_type::value_type value_type;
	typedef typename allocator_type::size_type size_type;

	//explicit stack(const allocator_type& _Al = allocator_type()) : c(_Al) {}
	allocator_type get_allocator() const { return (c.get_allocator()); }
	bool empty() const { return (c.empty()); }
	u32 size() const { return c.size(); }
	value_type& top() { return (c.back()); }
	const value_type& top() const { return (c.back()); }
	void push(const value_type& _X) { c.push_back(_X); }
	void pop() { c.pop_back(); }
	bool operator== (const xr_stack<_Ty, _C>& _X) const { return (c == _X.c); }
	bool operator!= (const xr_stack<_Ty, _C>& _X) const { return (!(*this == _X)); }
	bool operator< (const xr_stack<_Ty, _C>& _X) const { return (c < _X.c); }
	bool operator>(const xr_stack<_Ty, _C>& _X) const { return (_X < *this); }
	bool operator<= (const xr_stack<_Ty, _C>& _X) const { return (!(_X < *this)); }
	bool operator>= (const xr_stack<_Ty, _C>& _X) const { return (!(*this < _X)); }

protected:
	_C c;
};

template <typename T, typename allocator = xalloc<T> > class xr_list : public ::std::list < T, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename K, class P = ::std::less<K>, typename allocator = xalloc<K> > class xr_set : public ::std::set < K, P, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename K, class P = ::std::less<K>, typename allocator = xalloc<K> > class xr_multiset : public ::std::multiset < K, P, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename K, class V, class P = ::std::less<K>, typename allocator = xalloc<::std::pair<K, V> > > class xr_map : public ::std::map < K, V, P, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename K, class V, class P = ::std::less<K>, typename allocator = xalloc<::std::pair<K, V> > > class xr_multimap : public ::std::multimap < K, V, P, allocator > { public: u32 size() const { return (u32)__super::size(); } };

#ifdef STLPORT
template <typename V, class _HashFcn = ::std::hash<V>, class _EqualKey = ::std::equal_to<V>, typename allocator = xalloc<V> > class xr_hash_set : public ::std::hash_set < V, _HashFcn, _EqualKey, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename V, class _HashFcn = ::std::hash<V>, class _EqualKey = ::std::equal_to<V>, typename allocator = xalloc<V> > class xr_hash_multiset : public ::std::hash_multiset < V, _HashFcn, _EqualKey, allocator > { public: u32 size() const { return (u32)__super::size(); } };

template <typename K, class V, class _HashFcn = ::std::hash<K>, class _EqualKey = ::std::equal_to<K>, typename allocator = xalloc<::std::pair<K, V> > > class xr_hash_map : public ::std::hash_map < K, V, _HashFcn, _EqualKey, allocator > { public: u32 size() const { return (u32)__super::size(); } };
template <typename K, class V, class _HashFcn = ::std::hash<K>, class _EqualKey = ::std::equal_to<K>, typename allocator = xalloc<::std::pair<K, V> > > class xr_hash_multimap : public ::std::hash_multimap < K, V, _HashFcn, _EqualKey, allocator > { public: u32 size() const { return (u32)__super::size(); } };
#else
template <typename K, class V, class _Traits = ::stdext::hash_compare<K, ::std::less<K> >, typename allocator = xalloc<::std::pair<K, V> > > class xr_hash_map : public stdext::hash_map < K, V, _Traits, allocator > { public: u32 size() const { return (u32)__super::size(); } };
#endif // #ifdef STLPORT

#endif

template <class _Ty1, class _Ty2> inline ::std::pair<_Ty1, _Ty2> mk_pair(_Ty1 _Val1, _Ty2 _Val2) { return (::std::pair<_Ty1, _Ty2>(_Val1, _Val2)); }

struct pred_str : public ::std::binary_function < char*, char*, bool >
{
	IC bool operator()(const char* x, const char* y) const { return xr_strcmp(x, y) < 0; }
};
struct pred_stri : public ::std::binary_function < char*, char*, bool >
{
	IC bool operator()(const char* x, const char* y) const { return stricmp(x, y) < 0; }
};

// STL extensions
#define DEF_VECTOR(N,T) typedef xr_vector< T > N; typedef N::iterator N##_it;
#define DEF_LIST(N,T) typedef xr_list< T > N; typedef N::iterator N##_it;
#define DEF_DEQUE(N,T) typedef xr_deque< T > N; typedef N::iterator N##_it;
#define DEF_MAP(N,K,T) typedef xr_map< K, T > N; typedef N::iterator N##_it;

#define DEFINE_DEQUE(T,N,I) typedef xr_deque< T > N; typedef N::iterator I;
#define DEFINE_LIST(T,N,I) typedef xr_list< T > N; typedef N::iterator I;
#define DEFINE_VECTOR(T,N,I) typedef xr_vector< T > N; typedef N::iterator I;
#define DEFINE_MAP(K,T,N,I) typedef xr_map< K , T > N; typedef N::iterator I;
#define DEFINE_MAP_PRED(K,T,N,I,P) typedef xr_map< K, T, P > N; typedef N::iterator I;
#define DEFINE_MMAP(K,T,N,I) typedef xr_multimap< K, T > N; typedef N::iterator I;
#define DEFINE_SVECTOR(T,C,N,I) typedef svector< T, C > N; typedef N::iterator I;
#define DEFINE_SET(T,N,I) typedef xr_set< T > N; typedef N::iterator I;
#define DEFINE_SET_PRED(T,N,I,P) typedef xr_set< T, P > N; typedef N::iterator I;
#define DEFINE_STACK(T,N) typedef xr_stack< T > N;

#include "FixedVector.h"
#include "buffer_vector.h"

// auxilary definition
DEFINE_VECTOR(bool, boolVec, boolIt);
DEFINE_VECTOR(BOOL, BOOLVec, BOOLIt);
DEFINE_VECTOR(BOOL*, LPBOOLVec, LPBOOLIt);
DEFINE_VECTOR(Frect, FrectVec, FrectIt);
DEFINE_VECTOR(Irect, IrectVec, IrectIt);
DEFINE_VECTOR(Fplane, PlaneVec, PlaneIt);
DEFINE_VECTOR(Fvector2, Fvector2Vec, Fvector2It);
DEFINE_VECTOR(Fvector, FvectorVec, FvectorIt);
DEFINE_VECTOR(Fvector*, LPFvectorVec, LPFvectorIt);
DEFINE_VECTOR(Fcolor, FcolorVec, FcolorIt);
DEFINE_VECTOR(Fcolor*, LPFcolorVec, LPFcolorIt);
DEFINE_VECTOR(LPSTR, LPSTRVec, LPSTRIt);
DEFINE_VECTOR(LPCSTR, LPCSTRVec, LPCSTRIt);
DEFINE_VECTOR(string64, string64Vec, string64It);
DEFINE_VECTOR(xr_string, SStringVec, SStringVecIt);

DEFINE_VECTOR(s8, S8Vec, S8It);
DEFINE_VECTOR(s8*, LPS8Vec, LPS8It);
DEFINE_VECTOR(s16, S16Vec, S16It);
DEFINE_VECTOR(s16*, LPS16Vec, LPS16It);
DEFINE_VECTOR(s32, S32Vec, S32It);
DEFINE_VECTOR(s32*, LPS32Vec, LPS32It);
DEFINE_VECTOR(u8, U8Vec, U8It);
DEFINE_VECTOR(u8*, LPU8Vec, LPU8It);
DEFINE_VECTOR(u16, U16Vec, U16It);
DEFINE_VECTOR(u16*, LPU16Vec, LPU16It);
DEFINE_VECTOR(u32, U32Vec, U32It);
DEFINE_VECTOR(u32*, LPU32Vec, LPU32It);
DEFINE_VECTOR(float, FloatVec, FloatIt);
DEFINE_VECTOR(float*, LPFloatVec, LPFloatIt);
DEFINE_VECTOR(int, IntVec, IntIt);
DEFINE_VECTOR(int*, LPIntVec, LPIntIt);

#ifdef __BORLANDC__
DEFINE_VECTOR(AnsiString, AStringVec, AStringIt);
DEFINE_VECTOR(AnsiString*, LPAStringVec, LPAStringIt);
#endif

template <class T>
struct default_deleter
{
	void operator()(T* ptr) const
	{
		xr_delete(ptr);
	}
};

// smart pointer
template <typename T, typename D = default_deleter<T>>
class xptr
{
public:
										template <typename... Args>
										xptr									(Args&&... args)	{ m_data = xr_new<T>(_STD forward<Args>(args)...); }
										xptr									(nullptr_t)			{};

										xptr									(const xptr&)		= delete;
										xptr									(xptr&& old)		{ m_data = old.release(); }

										~xptr									()					{ reset(); }

private:
	T*									m_data									= nullptr;

public:
	xptr&								operator=								(const xptr&)		= delete;
	xptr&								operator=								(xptr&& old)		{ capture(old.release()); return *this; }

										operator bool							() const			{ return !!m_data; }
	T*									operator->								() const			{ R_ASSERT(m_data); return m_data; }
	T&									operator*								() const			{ R_ASSERT(m_data); return *m_data; }
	T*									get										() const			{ R_ASSERT(m_data); return m_data; }

	void								reset									()					{ D()(m_data); }
	void								capture									(T* p)				{ reset(); m_data = p; }
	T*									release									()					{ T* tmp = m_data; m_data = nullptr; return tmp; }
	
	template <typename... Args>
	xptr&								construct								(Args&&... args)	{ return construct<T>(_STD forward<Args>(args)...); }

	template <typename M, typename... Args>
	xptr&								construct								(Args&&... args)	{ capture(xr_new<M>(_STD forward<Args>(args)...)); return *this; }
};

#include <unordered_map>
template <typename key_type, typename value_type>
class xr_umap : public _STD unordered_map<key_type, value_type, _STD hash<key_type>, _STD equal_to<key_type>,
	xalloc<_STD pair<const key_type, value_type>>>
{
public:
	bool								contains								(const key_type& key)		{ return find(key) != end(); }
};

template <typename T>
class xoptional
{
	bool m_status;
	T m_data;

public:
	xoptional() : m_status{ false }, m_data{} {}
	xoptional(T CR$ data) : m_data { data }, m_status{ true } {}
	xoptional(T&& data) : m_data { data }, m_status{ true } {}

	xoptional& operator=(T CR$ data) { m_data = data; m_status = true; return *this; }
	xoptional& operator=(T&& data) { m_data = data; m_status = true; return *this; }

	explicit operator bool() const { return m_status; }
	T& operator*() { R_ASSERT(m_status); return m_data; }
	T* operator->() { R_ASSERT(m_status); return &m_data; }

	void set_status(bool status) { m_status = status; }
	void drop() { set_status(false); }

	T& get() { R_ASSERT(m_status); return m_data; }
	T& get_set() { m_status = true; return m_data; }
	T&& release() { R_ASSERT(m_status); m_status = false; return _STD move(m_data); }
};

template <typename T, size_t Size>
class xarr
{
	using Storage = typename std::aligned_storage<sizeof(T)* Size, alignof(T)>::type;

public:
	template <typename... Args>
	xarr(Args&&... args)
	{
		for (size_t i = 0; i < Size; ++i)
			new (get(i))T(_STD forward<Args>(args)...);
	}

	~xarr()
	{
		for (size_t i = 0; i < Size; ++i)
			(get(i))->~T();
	}

	xarr(const xarr& other)
	{
		for (size_t i = 0; i < Size; ++i)
			new (get(i))T(other[i]);
	}

	xarr(xarr&& other)
	{
		for (size_t i = 0; i < Size; ++i)
			new (get(i))T(_STD move(other[i]));
	}

	xarr& operator=(const xarr& other)
	{
		if (this != other)
		{
			for (size_t i = 0; i < Size; ++i)
			{
				get(i)->~T();
				new (get(i))T(other[i]);
			}
		}
		return *this;
	}

	xarr& operator=(xarr&& other)
	{
		if (this != other)
		{
			for (size_t i = 0; i < Size; ++i)
			{
				get(i)->~T();
				new (get(i))T(_STD move(other[i]));
			}
		}
		return *this;
	}

private:
	void check_range(size_t i) const noexcept { R_ASSERT2(i < Size, "Array out of range!"); }

public:
	T& operator[](size_t i) noexcept { check_range(i); return *get(i); }
	const T& operator[](size_t i) const noexcept { check_range(i); return *get(i); }

	constexpr size_t size() const noexcept { return Size; }

	T* data() noexcept { return reinterpret_cast<T*>(&m_data); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(&m_data); }

	T* get(size_t i) noexcept { return data() + i; }
	const T* get(size_t i) const noexcept { return data() + i; }

private:
	Storage m_data;
};
