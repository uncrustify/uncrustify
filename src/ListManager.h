/**
 * @file ListManager.h
 * Template class that manages items in a double-linked list.
 * If C++ could do it, this would just be a class that worked on an interface.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef LIST_MANAGER_H_INCLUDED
#define LIST_MANAGER_H_INCLUDED

#include "chunk.h"

/**
 * A simple list manager for a double-linked list of Chunk items.
 */
class ChunkListManager
{
protected:
   Chunk *m_head;  // pointer to the head of list
   Chunk *m_tail;  // pointer to tail of list

public:
   ChunkListManager()
   {
      m_head = Chunk::NullChunkPtr;
      m_tail = Chunk::NullChunkPtr;
   }


   /**
    * @return pointer to first element of the linked list
    */
   Chunk *GetHead() const
   {
      return(m_head);
   }


   /**
    * @return pointer to last element of the linked list
    */
   Chunk *GetTail() const
   {
      return(m_tail);
   }


   /**
    * @brief remove an element from a linked list
    * @param[in] obj chunk to remove from the list
    */
   void Remove(Chunk *obj)
   {
      if (obj != Chunk::NullChunkPtr)
      {
         if (m_head == obj)
         {
            m_head = obj->m_next;
         }

         if (m_tail == obj)
         {
            m_tail = obj->m_prev;
         }

         if (obj->m_next != Chunk::NullChunkPtr)
         {
            obj->m_next->m_prev = obj->m_prev;
         }

         if (obj->m_prev != Chunk::NullChunkPtr)
         {
            obj->m_prev->m_next = obj->m_next;
         }
         obj->m_next = Chunk::NullChunkPtr;
         obj->m_prev = Chunk::NullChunkPtr;
      }
   }


   //! swap two elements of a list
   void Swap(Chunk *obj1, Chunk *obj2)
   {
      if (  obj1 != Chunk::NullChunkPtr
         && obj2 != Chunk::NullChunkPtr)
      {
         if (obj1->m_prev == obj2)
         {
            Remove(obj1);
            AddBefore(obj1, obj2);
         }
         else if (obj2->m_prev == obj1)
         {
            Remove(obj2);
            AddBefore(obj2, obj1);
         }
         else
         {
            Chunk *m_prev1 = obj1->m_prev;
            Remove(obj1);

            Chunk *m_prev2 = obj2->m_prev;
            Remove(obj2);

            AddAfter(obj1, m_prev2);
            AddAfter(obj2, m_prev1);
         }
      }
   }


   /**
    * @brief add a new element after a reference position in a list
    *
    * @param obj  new element to add to list
    * @param ref  chunk after which to insert new object
    */
   void AddAfter(Chunk *obj, Chunk *ref)
   {
      if (  obj != Chunk::NullChunkPtr
         && ref != Chunk::NullChunkPtr)
      {
         obj->m_next = ref->m_next;
         obj->m_prev = ref;

         if (ref->m_next != Chunk::NullChunkPtr)
         {
            ref->m_next->m_prev = obj;
         }
         else
         {
            m_tail = obj;
         }
         ref->m_next = obj;
      }
   }


   /**
    * @brief add a new element before a reference position in a list
    *
    * @param obj  new element to add to list
    * @param ref  chunk before to insert new object
    */
   void AddBefore(Chunk *obj, Chunk *ref)
   {
      if (  obj != Chunk::NullChunkPtr
         && ref != Chunk::NullChunkPtr)
      {
         Remove(obj);
         obj->m_next = ref;
         obj->m_prev = ref->m_prev;

         if (ref->m_prev != Chunk::NullChunkPtr)
         {
            ref->m_prev->m_next = obj;
         }
         else
         {
            m_head = obj;
         }
         ref->m_prev = obj;
      }
   }


   /**
    * @brief add a new element to the tail of a lis
    *
    * @param obj  new element to add to the list
    */
   void AddTail(Chunk *obj)
   {
      obj->m_next = Chunk::NullChunkPtr;
      obj->m_prev = m_tail;

      if (m_tail == Chunk::NullChunkPtr)
      {
         m_tail = obj;
         m_head = obj;
      }
      else
      {
         m_tail->m_next = obj;
      }
      m_tail = obj;
   }


   /**
    * @brief add a new element to the head of a list
    *
    * @param obj  new element to add to the list
    */
   void AddHead(Chunk *obj)
   {
      obj->m_next = m_head;
      obj->m_prev = Chunk::NullChunkPtr;

      if (m_head == Chunk::NullChunkPtr)
      {
         m_tail = obj;
         m_head = obj;
      }
      else
      {
         m_head->m_prev = obj;
      }
      m_head = obj;
   }
};


#endif /* LIST_MANAGER_H_INCLUDED */
