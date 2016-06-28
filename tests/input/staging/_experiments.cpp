#include "FileSystemSpy.h"
#include "CrashReportsProviderDummy.h"
#include "AttachmentFactoryDummy.h"
#include "AttachmentFactorySpy.h"
#include "../src/UnityCrashLogsCollector.h"
#include "shims/logical/IsEmpty.h"
#include "shims/attribute/GetSize.h"

#include <UnitTest++.h>
#include <utility>

SUITE (UnityCrashLogsCollector)
{
	using namespace ::ureport;
	using namespace ::ureport::macosx;
	using namespace ::ureport::collectors;
	using namespace ::ureport::test;

	class FreshReport
	{
	public:
		std::unique_ptr<ReportDraft> m_Report;

		FreshReport ()
			: m_Report (CreateNewReport ())
		{
		}
	};

	class CrashReportsProviderStub : public CrashReportsProviderDummy
	{
	public:
		std::vector<CrashReport> m_Reports;

		std::vector< ::ureport::macosx::CrashReport> GetAllReports () const
		{
			return m_Reports;
		}
	};

	TEST_FIXTURE (FreshReport, AddsNoAttachmentWhenNoReportExists)
	{
		auto const provider = std::make_shared<CrashReportsProviderDummy> ();
		auto const factory = std::make_shared<AttachmentFactoryDummy> ();
		UnityCrashLogsCollector collector (provider, factory);
		collector.Collect (*m_Report);
		auto const attachments = m_Report->GetAttachments ();
		CHECK (IsEmpty (attachments));
	}

	TEST_FIXTURE (FreshReport, AddsUnityCrashLogsToReport)
	{
		auto const reports = std::make_shared<CrashReportsProviderStub> ();
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		auto const attachments = std::make_shared<AttachmentFactorySpy> ();
		UnityCrashLogsCollector collector (reports, attachments);
		collector.Collect (*m_Report);
		CHECK_EQUAL (2, GetSize (m_Report->GetAttachments ()));
		CHECK_EQUAL ("Unity.crash", attachments->m_Files.at (0).first);
		CHECK_EQUAL ("Unity.crash", attachments->m_Files.at (1).first);
	}

	TEST_FIXTURE (FreshReport, DoesNotAddNonUnityLogs)
	{
		auto const reports = std::make_shared<CrashReportsProviderStub> ();
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("NonUnity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("NonUnity.crash"));
		auto const attachments = std::make_shared<AttachmentFactorySpy> ();
		UnityCrashLogsCollector collector (reports, attachments);
		collector.Collect (*m_Report);
		CHECK_EQUAL (0, GetSize (m_Report->GetAttachments ()));
		CHECK_EQUAL (0, GetSize (attachments->m_Files));
	}

	TEST_FIXTURE (FreshReport, AddsCrashLogWithProperDescription)
	{
		auto const reports = std::make_shared<CrashReportsProviderStub> ();
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		auto const attachments = std::make_shared<AttachmentFactorySpy> ();
		UnityCrashLogsCollector collector (reports, attachments);
		collector.Collect (*m_Report);
		CHECK_EQUAL ("Unity crash log", attachments->m_Files.at (0).second);
	}

	TEST_FIXTURE (FreshReport, AddsMaximumTwoLogs)
	{
		auto const reports = std::make_shared<CrashReportsProviderStub> ();
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("NonUnity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("NonUnity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity.crash"));
		auto const attachments = std::make_shared<AttachmentFactoryDummy> ();
		UnityCrashLogsCollector collector (reports, attachments);
		collector.Collect (*m_Report);
		CHECK_EQUAL (2, GetSize (m_Report->GetAttachments ()));
	}

	TEST_FIXTURE (FreshReport, AddsRecentUnityLogs)
	{
		auto const reports = std::make_shared<CrashReportsProviderStub> ();
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity_01-01-010001.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity_01-01-010002.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity_01-01-010003.crash"));
		reports->m_Reports.push_back (CrashReport::MakeFromPath ("Unity_01-01-010004.crash"));
		auto const attachments = std::make_shared<AttachmentFactorySpy> ();
		UnityCrashLogsCollector collector (reports, attachments);
		collector.Collect (*m_Report);
		CHECK_EQUAL ("Unity_01-01-010004.crash", attachments->m_Files.at (0).first);
		CHECK_EQUAL ("Unity_01-01-010003.crash", attachments->m_Files.at (1).first);
	}
}
