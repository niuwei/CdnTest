
// init test unit include
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

#include <cppunit/config/SourcePrefix.h>
#include "MultiCdnStrategyTestCase.h"

mock_cdn_pack::mock_cdn_pack(std::string host)
: m_host(host)
, m_connect_state(CONNECT_WAITING)
{

}

mock_cdn_pack::~mock_cdn_pack()
{

}

mock_multi_cdn_strategy::mock_multi_cdn_strategy(vod_connect_dispatcher * dispatch_ptr, unsigned long bitrate)
: multi_cdn_strategy(dispatch_ptr, bitrate)
{

}

mock_multi_cdn_strategy::~mock_multi_cdn_strategy()
{
	clear_cdn_pack();
}

void mock_multi_cdn_strategy::clear_cdn_pack()
{
	std::vector<cdn_pack_base*>::iterator it = m_mock_cdn_packs.begin();
	for (; it != m_mock_cdn_packs.end(); ++ it)
	{
		cdn_pack_base * pack_ptr = *it;
		if (pack_ptr)
		{
			delete pack_ptr;
			pack_ptr = NULL;
		}
	}
	m_mock_cdn_packs.clear();
}

void mock_multi_cdn_strategy::get_query_cdn_result(std::list<std::string>& cdns)
{
	std::list<std::string>::iterator it = m_query_cdn_result.begin();
	for (; it != m_query_cdn_result.end(); ++ it)
	{
		cdns.push_back(*it);
	}
}

void mock_multi_cdn_strategy::get_all_cdns_from_dispacther(std::vector<cdn_pack_base*>& cdns)
{
	std::vector<cdn_pack_base*>::iterator it = m_mock_cdn_packs.begin();
	for (; it != m_mock_cdn_packs.end(); ++ it)
	{
		cdn_pack_base* pack_ptr = convert_to_cdn_pack(*it);
		cdns.push_back(pack_ptr);
	}
}

void mock_multi_cdn_strategy::get_connected_cdns_from_dispatcher(std::vector<cdn_pack_base*>& cdns)
{
	std::vector<cdn_pack_base*>::iterator it = m_mock_cdn_packs.begin();
	for (; it != m_mock_cdn_packs.end(); ++ it)
	{
		cdn_pack_base* pack_ptr = convert_to_cdn_pack(*it);
		if (pack_ptr->get_connect_state() == CONNECTED)
		{
			cdns.push_back(pack_ptr);
		}
	}
}

cdn_pack_base* mock_multi_cdn_strategy::convert_to_cdn_pack(cdn_pack_base* pack_ptr)
{
	cdn_pack_base * ret_ptr = NULL;

	std::vector<cdn_pack_base*>::iterator it = m_cdn_pack_pool.begin();
	for (; it != m_cdn_pack_pool.end(); ++ it)
	{
		cdn_pack_base* cdn_ptr = *it;
		if (cdn_ptr->is_equal(pack_ptr))
		{
			ret_ptr = *it;
			break;
		}
	}

	if (!ret_ptr)
	{
		ret_ptr = pack_ptr->copy();
		m_cdn_pack_pool.push_back(ret_ptr);
	}

	return ret_ptr;
}


CPPUNIT_TEST_SUITE_REGISTRATION( MultiCdnStrategyTestCase );

void MultiCdnStrategyTestCase::setUp()
{
	// ����
	m_multi_cdn_ptr = new mock_multi_cdn_strategy(NULL, 40000);
}

void MultiCdnStrategyTestCase::tearDown()
{
	delete m_multi_cdn_ptr;
}

void MultiCdnStrategyTestCase::TestInitState()
{
 	int state = m_multi_cdn_ptr->get_current_state();
 	CPPUNIT_ASSERT( state == state_uninit );
}

void MultiCdnStrategyTestCase::TestStateChange()
{
	int original_state = m_multi_cdn_ptr->get_current_state();

	// ����״̬����start_event��ת��
	m_multi_cdn_ptr->change_state(state_uninit, false);
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT( m_multi_cdn_ptr->get_current_state() == state_uninit );

	m_multi_cdn_ptr->change_state(state_stop, false);
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT( m_multi_cdn_ptr->get_current_state() == state_scale );

	m_multi_cdn_ptr->change_state(state_scale, false);
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT( m_multi_cdn_ptr->get_current_state() == state_scale );

	m_multi_cdn_ptr->change_state(state_compare, false);
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT( m_multi_cdn_ptr->get_current_state() == state_compare );

	m_multi_cdn_ptr->change_state(state_over, false);
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT( m_multi_cdn_ptr->get_current_state() == state_over );

	m_multi_cdn_ptr->change_state(original_state, false);
}

// �ӷ���������õ�3��ip�����Ѽ�����Դ��cdn��ͬ�����
// ӵ��3����ѡcdn������״̬state_stop
void MultiCdnStrategyTestCase::TestInitEventNormal()
{
	// ģ��query cdn��������ip
	m_multi_cdn_ptr->m_query_cdn_result.clear();
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.1");
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.2");
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.3");

	// ģ�����е�cdn��Դ
	m_multi_cdn_ptr->clear_cdn_pack();
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.3"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.2"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.1"));

	m_multi_cdn_ptr->init_event();

	int cdn_count = m_multi_cdn_ptr->m_alternate_cdns.size();
	CPPUNIT_ASSERT(cdn_count == 3);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_stop);

	// ��鱸ѡ�б��˳���Ƿ������󵽵�˳��һ��
	std::list<cdn_pack_base*>::iterator it = m_multi_cdn_ptr->m_alternate_cdns.begin();
	for (int i = 0; i < cdn_count; ++ i, ++ it)
	{
		cdn_pack_base * pack_ptr = *it;
		switch (i)
		{
		case 0:
			CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");
			break;
		case 1:
			CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.2");
			break;
		case 2:
			CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.3");
			break;
		default:
			CPPUNIT_FAIL("more than 3 elements");
			break;
		}
	}
}

// ӵ�е�cdn��Դ��query cdn������ip������
// �����Ǵ�ʱ���ڲ��Ź����У��ѽ�������������cdn������Դ
void MultiCdnStrategyTestCase::TestInitEventMoreCdnPipe()
{
	m_multi_cdn_ptr->m_query_cdn_result.clear();
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.1");
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.2");

	m_multi_cdn_ptr->clear_cdn_pack();
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.1"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.2"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.3"));

	m_multi_cdn_ptr->init_event();

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_alternate_cdns.size() == 2);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_stop);
}

// query����һ��cdn�����
void MultiCdnStrategyTestCase::TestInitEventSingleQueryResult()
{
	m_multi_cdn_ptr->m_query_cdn_result.clear();
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.1");

	m_multi_cdn_ptr->clear_cdn_pack();
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.1"));

	m_multi_cdn_ptr->init_event();

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_alternate_cdns.size() == 0);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_over);
}

// �����ˣ���һ�����Ҳû�е��쳣���
void MultiCdnStrategyTestCase::TestInitEventQueryError()
{
	m_multi_cdn_ptr->m_query_cdn_result.clear();

	m_multi_cdn_ptr->clear_cdn_pack();

	m_multi_cdn_ptr->init_event();

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_alternate_cdns.size() == 0);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_over);
}

// �����������һ��cdn
void MultiCdnStrategyTestCase::TestOpenCdnNormal()
{
	// ���ȳ�ʼ��
	TestInitEventNormal();

	// ģ��һ��cdn���������cdnӦ���϶�cdn����������
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 1);

	// ��cdn����������״̬state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);
}

// ����cdn��ֻ��һ������
void MultiCdnStrategyTestCase::TestOpenCdnSingleResult()
{
	// ���ȳ�ʼ��
	TestInitEventSingleQueryResult();

	// ģ��һ��cdn���������cdnӦ���϶�cdn����������
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 0); // Ψһ���õĲ���usable�У���default��
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_default_cdn->get_host() == "192.168.0.1");


	// ��cdn�����������ǽ���״̬state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_over);
}

// ����cdn��û�г�ʼ�������
void MultiCdnStrategyTestCase::TestOpenCdnNoInit()
{
	// û�г�ʼ��

	// ģ��һ��cdn���������cdnӦ���϶�cdn����������
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	CPPUNIT_ASSERT(pack_ptr == NULL);

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 0);

	// û����cdn����
	m_multi_cdn_ptr->start_event(); // ʲô������
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_uninit);
}

// ���Ե�vp����״̬�����������ٶ�С�����ʾͽ����vp����״̬
void MultiCdnStrategyTestCase::TestScaleStateNormal()
{
	// ���ȳ�ʼ��
	TestInitEventNormal();

	// ģ��һ��cdn���������cdnӦ���϶�cdn����������
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 1);

	// ��cdn����������״̬state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);

	// ģ��recv_data_success�и�sample�������ݵĹ��̣��Ȼ���n�롣
	// ��ʱ��2��һ�Σ���������Ϊ ʱ��/2
	int sample_count = 9; // 9��������18��
	for (int i = 0; i < 9; ++ i)
	{
		// ����ÿ��sample��������ռ���������
		m_multi_cdn_ptr->collect_download_event(pack_ptr, 40000);
		m_multi_cdn_ptr->collect_download_event(pack_ptr, 45000);
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_sample_cumulated == 40000 + 45000);
		m_multi_cdn_ptr->time_out_event(); // ��ʱ���¼�
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_sample_cumulated == 0);
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == 0);
	}
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 9);

	// �ٶ�һ�ζ�ʱ����Ϣ�����������ﵽ10������ʼ�����Ƿ��������
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 40000);
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 45000);
	m_multi_cdn_ptr->time_out_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 10);

	// ��ʱ���ۻ�������ʱ(40000+45000)*5�β��� ���� ����40000*10�룬����ԭ��״̬����������
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == (40000+45000)*5);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);

	m_multi_cdn_ptr->collect_download_event(pack_ptr, 30000);
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 20000);
	m_multi_cdn_ptr->time_out_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 10);

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == 390000); // (40000+45000)*4+30000+20000 ���С������40000*10
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_compare);  // ����״̬state_compare
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_comparing_cdns.size() == 2);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_useable_cdns.size() == usable_cdn_count);	// �Ƚ�״̬��û�����ӿ���cdn��

}

void MultiCdnStrategyTestCase::TestCompareStateNormal()
{

}


//////////////////////////////////////////////////////////////////////////
int run_multi_cdn_strategy_test_case()
{
	// Create the event manager and test controller
	CPPUNIT_NS::TestResult controller;

	// Add a listener that colllects test result
	CPPUNIT_NS::TestResultCollector result;
	controller.addListener( &result );        

	// Add a listener that print dots as test run.
	CPPUNIT_NS::BriefTestProgressListener progress;
	controller.addListener( &progress );      

	// Add the top suite to the test runner
	CPPUNIT_NS::TestRunner runner;
	runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
	runner.run( controller );

	// Print test in a compiler compatible format.
	std::ofstream ofs("c:\\MultiCdnTestResult.txt");
	CPPUNIT_NS::CompilerOutputter outputter( &result, ofs );
	outputter.write(); 

	return result.wasSuccessful() ? 0 : 1;
}