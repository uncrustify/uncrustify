/**
 * @file ListManager.h
 * Template class that manages items in a double-linked list.
 * If C++ could do it, this would just be a class that worked on an interface.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

/**
 * A simple list manager for a double-linked list.
 * Class T must define 'next' and 'prev', which must be pointers to type T.
 */

template<class T> class ListManager
{
protected:

   /* Pointers to the head and tail.
    * They are either both NULL or both non-NULL.
    */
   T *first;
   T *last;

private:
   /* Hide copy constructor */
   ListManager(const ListManager& ref)
   {
      first = NULL;
      last  = NULL;
   }


public:
   ListManager()
   {
      first = NULL;
      last  = NULL;
   }


   T *GetHead()
   {
      return(first);
   }


   T *GetTail()
   {
      return(last);
   }


   T *GetNext(T *ref)
   {
      return((ref != NULL) ? ref->next : NULL);
   }


   T *GetPrev(T *ref)
   {
      return((ref != NULL) ? ref->prev : NULL);
   }


   void InitEntry(T *obj) const
   {
      if (obj != NULL)
      {
         obj->next = NULL;
         obj->prev = NULL;
      }
   }


   void Pop(T *obj)
   {
      if (obj != NULL)
      {
         if (first == obj)
         {
            first = obj->next;
         }
         if (last == obj)
         {
            last = obj->prev;
         }
         if (obj->next != NULL)
         {
            obj->next->prev = obj->prev;
         }
         if (obj->prev != NULL)
         {
            obj->prev->next = obj->next;
         }
         obj->next = NULL;
         obj->prev = NULL;
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
         obj->next = ref->next;
         obj->prev = ref;
         if (ref->next != NULL)
         {
            ref->next->prev = obj;
         }
         else
         {
            last = obj;
         }
         ref->next = obj;
      }
   }


   void AddBefore(T *obj, T *ref)
   {
      if ((obj != NULL) && (ref != NULL))
      {
         Pop(obj);
         obj->next = ref;
         obj->prev = ref->prev;
         if (ref->prev != NULL)
         {
            ref->prev->next = obj;
         }
         else
         {
            first = obj;
         }
         ref->prev = obj;
      }
   }


   void AddTail(T *obj)
   {
      obj->next = NULL;
      obj->prev = last;
      if (last == NULL)
      {
         last  = obj;
         first = obj;
      }
      else
      {
         last->next = obj;
      }
      last = obj;
   }


   void AddHead(T *obj)
   {
      obj->next = first;
      obj->prev = NULL;
      if (first == NULL)
      {
         last  = obj;
         first = obj;
      }
      else
      {
         first->prev = obj;
      }
      first = obj;
   }
};
