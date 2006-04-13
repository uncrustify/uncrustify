#include <cstdio>

class ChunkNode
{
public:
	ChunkNode() {
		pc = NULL;
		seqnum = 0;
		prev = next = this;
	}

	ChunkNode(const ChunkNode& cn) {
		pc = cn.pc;
		seqnum = cn.seqnum;
		prev = next = this;
	}

	ChunkNode(void *c, int s=0) {
		pc = c;
		seqnum = s;
		prev = next = this;
	}

	ChunkNode	*prev;
	ChunkNode	*next;
	int			seqnum;
	void		*pc;
};

template <class T> class ListManager
{
protected:
	T head;

public:
	ListManager()
	{
		head.next = head.prev = &head;
	}
	
	ListManager(const ListManager& ref)
	{
		head.next = head.prev = &head;
	}

	T *GetHead()
	{
		return GetNext(&head);
	}

	T *GetTail()
	{
		return GetPrev(&head);
	}

	T *GetNext(T *ref)
	{
		return (ref->next != &head) ? ref->next : NULL;
	}

	T *GetPrev(T *ref)
	{
		return (ref->prev != &head) ? ref->prev : NULL;
	}

	void Pop(T *ref)
	{
		if (ref->next != ref)
		{
			ref->next->prev = ref->prev;
			ref->prev->next = ref->next;
		}
		ref->next = ref->prev = ref;
	}

	void AddAfter(T* obj, T* ref)
	{
		Pop(obj);
		obj->next = ref->next;
		obj->prev = ref;
		ref->next->prev = obj;
		ref->next = obj;
	}

	void AddBefore(T* obj, T* ref)
	{
		Pop(obj);
		obj->next = ref;
		obj->prev = ref->prev;
		ref->prev->next = obj;
		ref->prev = obj;
	}

	void AddTail(T* obj)
	{
		AddBefore(obj, &head);
	}

	void AddHead(T* obj)
	{
		AddAfter(obj, &head);
	}
};

typedef ListManager<ChunkNode> ChunkNodeList;


int main(int argc, char **argv)
{
	ChunkNodeList cnl;
	ChunkNode d1(NULL, 1);
	ChunkNode d2(NULL, 2);
	ChunkNode d3(NULL, 3);

	cnl.AddTail(&d1);
	cnl.AddTail(&d2);
	cnl.AddHead(&d3);

	ChunkNode *p;

	for (p = cnl.GetHead(); p != NULL; p = cnl.GetNext(p))
	{
		printf("sn=%d\n", p->seqnum);
	}
	
	return 0;
}
