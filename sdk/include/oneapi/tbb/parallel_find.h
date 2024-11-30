#ifndef __TBB_parallel_find_H
#define __TBB_parallel_find_H
#pragma warning (disable: 4180)
#include "parallel_for.h"
#include "blocked_range.h"

namespace tbb
{
    namespace detail
    {
        namespace d1
        {
            template<typename Iter, typename T>
            struct parallel_find_helper
            {
                task_group_context& context;
                std::atomic<std::size_t>& index;
                const T& value;
                Iter begin;

                parallel_find_helper(task_group_context& c, std::atomic<std::size_t>& i, const T& t, Iter b)
                    : context(c), index(i), value(t), begin(b) {}

                void operator()(blocked_range<Iter> range) const 
                {
                    Iter iter = std::find(range.begin(), range.end(), value);
                    if (iter != range.end())
                    {
                        index = std::distance(begin, iter);
                        context.cancel_group_execution();
                    }
                }
            };

            template<typename Iter, typename T>
            __TBB_requires(std::input_iterator<Iter>&& parallel_for_each_iterator_body<T, Iter>)
            Iter parallel_find(Iter begin, Iter end, const T& value)
            {
                task_group_context context;
                std::atomic<std::size_t> index;
                index = std::numeric_limits<std::size_t>::max();

                parallel_find_helper<Iter, T> worker(context, index, value, begin);

                parallel_for(blocked_range<Iter>(begin, end, 10000), worker, context);

                std::size_t result_index = index.load();
                return result_index == std::numeric_limits<std::size_t>::max() ? end : std::next(begin, result_index);
            }

            template<typename _Ty, typename T>
             __TBB_requires(container_based_sequence<_Ty, std::input_iterator_tag> && parallel_for_each_range_body<T, _Ty>)
            decltype(auto) parallel_find(_Ty & obj, const T& value)
            {
                return parallel_find(std::begin(obj), std::end(obj), value);
            }
        }
    }

    inline namespace v1
    {
        using detail::d1::parallel_find;
    } // namespace v1

} // namespace tbb

#endif /*__TBB_parallel_find_H*/