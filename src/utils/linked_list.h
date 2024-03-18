#ifndef LINKED_LIST_quard___fdsnjfdnskifds498dfs465fds465nhgk
#define LINKED_LIST_quard___fdsnjfdnskifds498dfs465fds465nhgk


namespace markussecundus::utils::linked_lists{

    template<typename TAccessPolicy, typename TNode>
    concept linked_list_manipulator_access_policy = requires(TAccessPolicy pol, TNode a, TNode b)
    {
        {pol.get_next(a)} -> std::convertible_to<TNode>;
        {pol.get_last(a)} -> std::convertible_to<TNode>;
        {pol.set_next(a, b)} -> std::convertible_to<void>;
        {pol.set_last(a, b)} -> std::convertible_to<void>;
        {pol.is_same_node(a, b)} ->std::convertible_to<bool>;
        {pol.is_null(a)} -> std::convertible_to<bool>;
    };

    template<typename TNode, linked_list_manipulator_access_policy<TNode> TNodeAccessPolicy>
    class linked_list_manipulator_t : private TNodeAccessPolicy {
    private:
        using p = TNodeAccessPolicy;
    public:
        template<typename ...Args>
        linked_list_manipulator_t(Args ...args) : TNodeAccessPolicy(args...) {}

        TNode init_node(TNode a) {
            if (p::is_null(a)) return a;
            p::set_next(a, a);
            p::set_last(a, a);
            return a;
        }

        TNode next(TNode n) {
            return p::get_next(n);
        }
        TNode last(TNode n) {
            return p::get_last(n);
        }


        TNode insert_list(TNode n, TNode to_append) {
            if (p::is_null(n)) return to_append;
            if (p::is_null(to_append)) return n;

            if (p::is_same_node(n, to_append))
                return n;

            TNode n_next = p::get_next(n);
            TNode to_append_last = p::get_last(to_append);

            p::set_next(n, to_append);
            p::set_last(to_append, n);
            p::set_next(to_append_last, n_next);
            p::set_last(n_next, to_append_last);
            return n;
        }

        TNode prepend_list(TNode a, TNode b) {
            if (p::is_null(a)) return b;
            if (p::is_null(b)) return a;

            insert_list(p::get_last(a), b);
            return a;
        }

        void swap_nodes(TNode a, TNode b) {
            if (p::is_null(a)) return;
            if (p::is_null(b)) return;

            bool a_is_single = is_single_node(a);
            bool b_is_single = is_single_node(b);

            TNode a_remainder = disconnect_node(a);
            TNode b_remainder = disconnect_node(b);

            if (!a_is_single) prepend_list(a_remainder, b);
            if (!b_is_single) prepend_list(b_remainder, a);
        }

        TNode disconnect_node(TNode a) {
            if (p::is_null(a)) return a;

            TNode last = p::get_last(a);
            TNode next = p::get_next(a);

            p::set_next(last, next);
            p::set_last(next, last);

            init_node(a);

            return next;
        }

        bool is_single_node(TNode a) {
            return p::is_same_node(a, p::get_next(a));
        }

        std::size_t length(TNode a) {
            std::size_t ret = 0;
            for_each(a, [&](TNode _) {(void)_; ++ret; });
            return ret;
        }

        bool validate_list(TNode a) {
            bool ret = true;
            for_each(a, [&](TNode a) {
                if (!p::is_same_node(a, p::get_last(p::get_next(a)))) ret = false;
                if (!p::is_same_node(a, p::get_next(p::get_last(a)))) ret = false;
                });
            return ret;
        }


        template<typename TFunc>
        void for_each(TNode begin, TFunc iteration) {
            TNode a = begin, last = p::get_last(begin);
            for (;;) {
                TNode next = p::get_next(a); //must fetch before iteration() makes some potentiall destructive changes
                iteration(a);
                if (p::is_same_node(a, last)) //a destructive change might occur on begin meaning we might not come back to the start, but we are still guaranteed to reach the very last element of the list at some point 
                    break; 
                a = next;
            }
        }
    };
}




#endif