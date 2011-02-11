
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
	// 码率
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

	// 各种状态对于start_event的转换
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

// 从服务器请求得到3个ip，和已加入资源的cdn相同的情况
// 拥有3个可选cdn，进入状态state_stop
void MultiCdnStrategyTestCase::TestInitEventNormal()
{
	// 模拟query cdn请求来的ip
	m_multi_cdn_ptr->m_query_cdn_result.clear();
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.1");
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.2");
	m_multi_cdn_ptr->m_query_cdn_result.push_back("192.168.0.3");

	// 模拟现有的cdn资源
	m_multi_cdn_ptr->clear_cdn_pack();
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.3"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.2"));
	m_multi_cdn_ptr->m_mock_cdn_packs.push_back(new mock_cdn_pack("192.168.0.1"));

	m_multi_cdn_ptr->init_event();

	int cdn_count = m_multi_cdn_ptr->m_alternate_cdns.size();
	CPPUNIT_ASSERT(cdn_count == 3);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_stop);

	// 检查备选列表的顺序是否与请求到的顺序一致
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

// 拥有的cdn资源比query cdn拉到的ip多的情况
// 可能是此时已在播放过程中，已将域名解析到的cdn加入资源
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

// query请求到一个cdn的情况
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

// 返回了，但一个结果也没有的异常情况
void MultiCdnStrategyTestCase::TestInitEventQueryError()
{
	m_multi_cdn_ptr->m_query_cdn_result.clear();

	m_multi_cdn_ptr->clear_cdn_pack();

	m_multi_cdn_ptr->init_event();

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_alternate_cdns.size() == 0);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_over);
}

// 正常情况启动一个cdn
void MultiCdnStrategyTestCase::TestOpenCdnNormal()
{
	// 首先初始化
	TestInitEventNormal();

	// 模拟一个cdn启动，这个cdn应符合多cdn的启动规则
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 1);

	// 有cdn启动，进入状态state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);
}

// 启动cdn，只有一个可用
void MultiCdnStrategyTestCase::TestOpenCdnSingleResult()
{
	// 首先初始化
	TestInitEventSingleQueryResult();

	// 模拟一个cdn启动，这个cdn应符合多cdn的启动规则
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 0); // 唯一可用的不在usable中，在default中
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_default_cdn->get_host() == "192.168.0.1");


	// 有cdn启动，但不是进入状态state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_over);
}

// 启动cdn，没有初始化的情况
void MultiCdnStrategyTestCase::TestOpenCdnNoInit()
{
	// 没有初始化

	// 模拟一个cdn启动，这个cdn应符合多cdn的启动规则
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	CPPUNIT_ASSERT(pack_ptr == NULL);

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 0);

	// 没有有cdn启动
	m_multi_cdn_ptr->start_event(); // 什么都不做
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_uninit);
}

// 测试单vp测速状态的情况，如果速度小于码率就进入多vp测速状态
void MultiCdnStrategyTestCase::TestScaleStateNormal()
{
	// 首先初始化
	TestInitEventNormal();

	// 模拟一个cdn启动，这个cdn应符合多cdn的启动规则
	cdn_pack_base * pack_ptr = m_multi_cdn_ptr->get_open_cdn();
	pack_ptr->open();
	CPPUNIT_ASSERT(pack_ptr->get_host() == "192.168.0.1");

	int usable_cdn_count = m_multi_cdn_ptr->m_useable_cdns.size();
	CPPUNIT_ASSERT(usable_cdn_count == 1);

	// 有cdn启动，进入状态state_scale
	m_multi_cdn_ptr->start_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);

	// 模拟recv_data_success中给sample积累数据的过程，先积累n秒。
	// 定时器2秒一次，样本数量为 时间/2
	int sample_count = 9; // 9个样本，18秒
	for (int i = 0; i < 9; ++ i)
	{
		// 假设每个sample间隔可以收集两次数据
		m_multi_cdn_ptr->collect_download_event(pack_ptr, 40000);
		m_multi_cdn_ptr->collect_download_event(pack_ptr, 45000);
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_sample_cumulated == 40000 + 45000);
		m_multi_cdn_ptr->time_out_event(); // 定时器事件
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_sample_cumulated == 0);
		CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == 0);
	}
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 9);

	// 再多一次定时器消息，样本数量达到10个，开始计算是否大于码率
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 40000);
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 45000);
	m_multi_cdn_ptr->time_out_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 10);

	// 这时的累积下载量时(40000+45000)*5次采样 大于 码率40000*10秒，保留原来状态，继续采样
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == (40000+45000)*5);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_scale);

	m_multi_cdn_ptr->collect_download_event(pack_ptr, 30000);
	m_multi_cdn_ptr->collect_download_event(pack_ptr, 20000);
	m_multi_cdn_ptr->time_out_event();
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_sample_list.size() == 10);

	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_scale_total_down == 390000); // (40000+45000)*4+30000+20000 这个小于码率40000*10
	CPPUNIT_ASSERT(m_multi_cdn_ptr->get_current_state() == state_compare);  // 进入状态state_compare
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_comparing_cdns.size() == 2);
	CPPUNIT_ASSERT(m_multi_cdn_ptr->m_useable_cdns.size() == usable_cdn_count);	// 比较状态中没有增加可用cdn数

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