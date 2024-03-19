#ifndef LINKED_LIST_quard___fdsnjfdnskifds498dfs465fds465nhgk
#define LINKED_LIST_quard___fdsnjfdnskifds498dfs465fds465nhgk


namespace markussecundus::utils::linked_lists{

    /// <summary>
    /// Defines a facade for low-level manipulation of any kind of object as a doubly linked list. 
    /// To be used by linked_list_manipulator_t which creates cyclic linked list on top of it with provided high-level manipulation commands.
    /// </summary>
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

    /// <summary>
    /// Facade that implements cyclic double-linked list functionality on top of any node type. 
    /// Only basic setters for last/next node must be provided as a template parameter.
    /// 
    /// Note: this heavy abstraction was pretty much necessary for the linked list 
    ///       logic to be both usable in queue_pool and reasonably testable.
    /// </summary>
    /// <typeparam name="TNode">Node type of the linked list</typeparam>
    /// <typeparam name="TNodeAccessPolicy">Facade providing implementations of low-level operations to be performed on the nodes.</typeparam>
    template<typename TNode, linked_list_manipulator_access_policy<TNode> TNodeAccessPolicy>
    class linked_list_manipulator_t : private TNodeAccessPolicy {
    private:
        using p = TNodeAccessPolicy;
    public:
        template<typename ...Args>
        linked_list_manipulator_t(Args ...args) : TNodeAccessPolicy(args...) {}

        /// <summary>
        /// Initializes the node into a valid size cyclic linked list of size 1 - both its next and last pointer pointing to itself.
        /// </summary>
        /// <param name="a">Node to be initialized</param>
        /// <returns>The same node for chaining purposes</returns>
        TNode init_node(TNode a) {
            if (p::is_null(a)) return a;
            p::set_next(a, a);
            p::set_last(a, a);
            return a;
        }

        /// <summary>
        /// Gets next node of the linked list.
        /// If provided node is considered to be the list's tail, it wraps back to the list's first node.
        /// </summary>
        TNode next(TNode n) {
            return p::get_next(n);
        }
        /// <summary>
        /// Gets previous node of the linked list.
        /// If provided node is considered to be the list's head, that corresponds to the last node of the list.
        /// </summary>
        TNode last(TNode n) {
            return p::get_last(n);
        }

        /// <summary>
        /// Concatenates another list right behind the current node in O(1) time.
        /// E.g. if lists are:
        /// a... 1->2->3->4
        /// b... 7->8,
        /// then the result is 1->7->8->3->4
        /// 
        /// If both arguments point to different nodes of the same list, then the behaviour is undefined!
        /// </summary>
        /// <param name="n">Head of the linked list</param>
        /// <param name="to_append">List to be concatenated into the main list</param>
        /// <returns>Head of the list for chaining purposes</returns>
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

        /// <summary>
        /// Joins two linked lists in O(1) time.
        /// E.g. if lists are:
        /// a... 1->2->3->4
        /// b... 7->8,
        /// then the result is 7->8->1->2->3->4 (from the point of view of b)
        /// or viewed from a's point of view: 1->2->3->4->7->8 (the list is cyclic afterall)
        /// 
        /// If both arguments point to different nodes of the same list, then the behaviour is undefined!
        /// </summary>
        /// <param name="n">First linked list to be joined</param>
        /// <param name="to_append">Second list to be joined</param>
        /// <returns>Head of the list `a` for chaining purposes</returns>
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

        /// <summary>
        /// Extracts the provided node from its list, decreasing its length by 1.
        /// The extracted node gets initialized to a valid single-node linked list.
        /// </summary>
        /// <param name="a">Node to be extracted.</param>
        /// <returns>Head of the remaining list. Might be the extracted node itself if it already was alone in its list.</returns>
        TNode disconnect_node(TNode a) {
            if (p::is_null(a)) return a;

            TNode last = p::get_last(a);
            TNode next = p::get_next(a);

            p::set_next(last, next);
            p::set_last(next, last);

            init_node(a);

            return next;
        }

        /// <summary>
        /// Checks if the length of the linked list is exactly 1 (e.g. if it was just initialized).
        /// </summary>
        /// <param name="a">Node to be checked</param>
        /// <returns>`true` IFF the node is alone in its list</returns>
        bool is_single_node(TNode a) {
            return p::is_same_node(a, p::get_next(a));
        }

        /// <summary>
        /// Computes how many nodes there are in the list in O(n) time.
        /// Returns 0 only if the provided node is null.
        /// </summary>
        /// <param name="a">List whose length to be computed.</param>
        /// <returns>Length of the list</returns>
        std::size_t length(TNode a) {
            std::size_t ret = 0;
            for_each(a, [&](TNode _) {(void)_; ++ret; });
            return ret;
        }

        /// <summary>
        /// Iterates through the list and checks if local integrity is not broken (a.k.a. `this->next->last == this` etc.).
        /// Mainly for testing purposes.
        /// </summary>
        /// <param name="a"></param>
        /// <returns>`true` if the list is ok</returns>
        bool validate_list(TNode a) {
            bool ret = true;
            for_each(a, [&](TNode a) {
                if (!p::is_same_node(a, p::get_last(p::get_next(a)))) ret = false;
                if (!p::is_same_node(a, p::get_next(p::get_last(a)))) ret = false;
                });
            return ret;
        }

        /// <summary>
        /// Iterates all nodes of a linked list, performing some user specified logic.
        /// 
        /// If the iteration logic makes destructive changes on the nodes that are being iterated, behaviour is undefined!
        /// </summary>
        /// <typeparam name="TFunc">Function to be invoked on each node.</typeparam>
        /// <param name="begin">Head node of the list</param>
        /// <param name="iteration">Function to be invoked on each node.</param>
        template<typename TFunc>
        void for_each(TNode begin, TFunc iteration) {
            TNode a = begin, last = p::get_last(begin);
            for (;;) {
                TNode next = p::get_next(a); //must fetch before iteration() makes some potential destructive changes
                iteration(a);
                if (p::is_same_node(a, last)) //a destructive change might occur on begin meaning we might not come back to the start, but we are still guaranteed to reach the very last element of the list at some point 
                    break; 
                a = next;
            }
        }
    };
}




#endif