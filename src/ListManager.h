/**
 * @file ListManager.h
 * Template class that manages items in a double-linked list.
 * If C++ could do it, this would just be a class that worked on an interface.
 *
 * $Id$
 */

/**
 * A simple list manager.
 * Class T must define 'next' and 'prev', which must be pointers to type T.
 */

template <class T> class ListManager
{
protected:
   /* I hope T isn't big, 'cause this declares a copy of it */
   T head;

public:
   ListManager()
   {
      head.next = head.prev = &head;
   }

   ListManager(const ListManager& ref)
   {
      /* TODO: can't copy the list... what to do?? */
      head.next = head.prev = &head;
   }

   T *GetHead()
   {
      return(GetNext(&head));
   }

   T *GetTail()
   {
      return(GetPrev(&head));
   }

   T *GetNext(T *ref)
   {
      return(((ref != NULL) && (ref->next != &head)) ? ref->next : NULL);
   }

   T *GetPrev(T *ref)
   {
      return(((ref != NULL) && (ref->prev != &head)) ? ref->prev : NULL);
   }

   void InitEntry(T *obj) const
   {
      if (obj != NULL)
      {
         obj->next = obj->prev = obj;
      }
   }

   void Pop(T *ref)
   {
      if (ref != NULL)
      {
         if (ref->next != ref)
         {
            ref->next->prev = ref->prev;
            ref->prev->next = ref->next;
         }
         ref->next = ref->prev = ref;
      }
   }

   void Swap(T *obj1, T *obj2)
   {
      if ((obj1 != NULL) && (obj2 != NULL))
      {
         if (obj1->prev == obj2)
         {
            Pop(obj1);
            AddBefore(obj1, obj2);
         }
         else if (obj2->prev == obj1)
         {
            Pop(obj2);
            AddBefore(obj2, obj1);
         }
         else
         {
            T *prev1 = obj1->prev;
            Pop(obj1);

            T *prev2 = obj2->prev;
            Pop(obj2);

            AddAfter(obj1, prev2);
            AddAfter(obj2, prev1);
         }
      }
   }

   void AddAfter(T *obj, T *ref)
   {
      if ((obj != NULL) && (ref != NULL))
      {
         Pop(obj);
         obj->next       = ref->next;
         obj->prev       = ref;
         ref->next->prev = obj;
         ref->next       = obj;
      }
   }

   void AddBefore(T *obj, T *ref)
   {
      if ((obj != NULL) && (ref != NULL))
      {
         Pop(obj);
         obj->next       = ref;
         obj->prev       = ref->prev;
         ref->prev->next = obj;
         ref->prev       = obj;
      }
   }

   void AddTail(T *obj)
   {
      AddBefore(obj, &head);
   }

   void AddHead(T *obj)
   {
      AddAfter(obj, &head);
   }
};
