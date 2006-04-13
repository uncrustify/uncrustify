
#include <stdio.h>


/** A smart node on a list */
template <class T> class DoubleLinkedNode
{
public:
   DoubleLinkedNode(const T& d)
   {
      data = d;
      next = prev = NULL;
   }

   /** Removes self from the list */
   void Pop()
   {
      if (next != NULL)
      {
         next->prev = prev;
         prev->next = next;
      }
      next = prev = NULL;
   }

   /** Adds self after ref */
   void AddAfter(DoubleLinkedNode *ref)
   {
      Pop();
      prev = ref;
      next = ref->next;
      if (next)
      {
         next->prev = this;
      }
      ref->next  = this;
   }

   /** Adds self before ref */
   void AddBefore(DoubleLinkedNode *ref)
   {
      Pop();
      next = ref;
      prev = ref->prev;
      if (prev)
      {
         prev->next = this;
      }
      ref->prev  = this;
   }

   DoubleLinkedNode *Next()
   {
      return next;
   }

   DoubleLinkedNode *Prev()
   {
      return prev;
   }

protected:
   DoubleLinkedNode *next;
   DoubleLinkedNode *prev;
public:
   T                *data;
};

class Dummy : public DoubleLinkedNode<Dummy>
{
public:
   int value;

   Dummy(int v=0) : value(v) { };

   //~Dummy()
   //{
   //};
};

int main()
{
   Dummy d1(1);
   Dummy d2(2);
   Dummy d3(3);

   Dummy head;

   d1.AddBefore(&head);
   d2.AddBefore(&head);
   d3.AddBefore(&head);
   d2.AddBefore(&head);

   Dummy *tmp;
   for (tmp = (Dummy *)head.Next(); tmp != &head; tmp = (Dummy *)tmp->Next())
   {
      printf("value=%d\n", tmp->value);
   }

   return 0;
}


// // NOTE for this hacked-up template to work, the class T must have
// //      a 'next' and 'prev' member that it doesn't touch.
// template <class T> class DoubleLinkedList
// {
// public:
//    struct Node
//    {
//       Node(const T& data, Node *next=this, Node *prev=this) :
//          data(data), next(next), prev(prev) { }
//
//       Node* next;
//       Node* prev;
//       T     data;
//    };
//
//    DoubleLinkedList() {}
//
//    DoubleLinkedList(const DoubleLinkedList& L)
//    {
//       for (const Node* i = L.m_head.next; i != L.m_head.prev; i = i->next)
//       {
//          AddTail(i->data);
//       }
//    }
//
//    Node *GetHead()
//    {
//       return (m_head.next != &m_head) ? m_head.next : null;
//    }
//
//    void Pop(Node *n)
//    {
//
//    }
//
//    void PopHead()
//    {
//       if (m_head.next != &m_head) ? m_head.next : null;
//    }
//
//    Node *GetTail()
//    {
//       return (m_head.prev != &m_head) ? m_head.prev : null;
//    }
//
//    Node *GetNext(Node *n)
//    {
//       return (n->next != &m_head) ? n->next : null;
//    }
//
//    Node *GetPrev(Node *n)
//    {
//       return (n->prev != &m_head) ? n->prev : null;
//    }
//
//    bool IsEmpty()
//    {
//       return (m_head.next == &m_head);
//    }
//
//    void AddTail(const T& data)
//    {
//       m_head.prev = new Node(data, &m_head, m_head.prev);
//    }
//
//    void AddHead(const T& data)
//    {
//       m_head.next = new Node(data, m_head.next, &m_head);
//    }
//
//    void Clear()
//    {
//       while (!IsEmpty())
//       {
//          RemoveHead();
//       }
//    }
//
//    ~List() { clear();}
//    void clear() { while (!empty()) pop_front();}
//
//    bool empty() { return (! head);}
//
//    void push_front(const T& x) {
//       Node* tmp = new Node(x,head);
//       head = tmp;
//    }
//
//    void pop_front() {
//       if (head)
//       {
//          Node* tmp = head; head=head->next; delete tmp;
//       }
//    }
//
//    void insert_after(Node* x, const T& data)
//    {
//       Node* tmp = new Node(data, x->next);
//       x->next = tmp;
//    }
//
//    void erase_after(Node* x)
//    {
//       Node* tmp = x->next;
//       if (tmp)
//       {
//          x->next = x->next->next;
//          delete tmp;
//       }
//    }
//
//
//    T& front() { return (head->data);}
//    const T& front() const { return (head->data);}
//
//    Node* begin() { return (head);}
//    Node* end() { return (0);}
//
//    const Node* begin() const { return (head);}
//    const Node* end() const { return (0);}
//
// private:
//    Node m_head;
// };
//
