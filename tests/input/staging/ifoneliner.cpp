#define REPORT_SEM_ERROR(action) ErrorStringMsg ("Failed to %s a semaphore (%s)\n", action, strerror (errno))

	inline void PlatformSemaphore::Create() { if (sem_init(&m_Semaphore, 0, 0) == -1) REPORT_SEM_ERROR ("open"); }
	inline void PlatformSemaphore::Destroy() { if (sem_destroy(&m_Semaphore) == -1) REPORT_SEM_ERROR ("destroy"); }


template<class T, class Compare, class Allocator>
class sorted_vector : private Compare
{
	public:

	typedef				std::vector<T, Allocator>			container;
	typedef typename	container::iterator					iterator;
	typedef typename	container::const_iterator			const_iterator;
	typedef typename	container::reverse_iterator			reverse_iterator;
	typedef typename	container::const_reverse_iterator	const_reverse_iterator;
	typedef typename	container::value_type				value_type;
	typedef typename	container::size_type				size_type;
	typedef typename	container::difference_type			difference_type;
	typedef				Compare								value_compare;
	typedef typename	Allocator::reference				reference;
	typedef typename	Allocator::const_reference			const_reference;
	typedef				Allocator							allocator_type;

};

class IndentOK
{
public:
	IndentOK ()
		: m_Factory (std::make_shared<AttachmentFactoryDummy> ())
		, m_Controller (CreateComposeReportController (
							m_Factory,
							std::make_shared<FileSystemDummy> (),
							[] (ReportDraft&, LongTermOperObserver*) {},
							[] (Report&, LongTermOperObserver*) {}))
		, m_Report (CreateNewReport ())
	{
		m_Controller->SetReport (m_Report);
	}
}

void foo()
{
	if (!GetInCache(i1, vertexInCache)) { AddToCache(i1, vertexInCache); cachedVerts++; m_cacheMisses++; } else m_cacheHits++;
	if(a) a++;

	if(b)
	{ b++ }
	else if(c)
	{ c++ }
	else { d++ }
}
