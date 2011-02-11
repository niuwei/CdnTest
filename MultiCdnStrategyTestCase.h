
#pragma once

#include <cppunit/extensions/HelperMacros.h>
#include "dispatcher/multi_cdn_strategy.h"
//#include "../qvs/qvs_Mars_V2.0.x/dispatcher/multi_cdn_strategy.h"

class cdn_pack_base;
class multi_cdn_strategy;

class mock_cdn_pack : public cdn_pack_base
{
public:
	mock_cdn_pack(std::string host);
	virtual ~mock_cdn_pack();

	virtual std::string get_host() { return m_host; }
	virtual cdn_pack_base* copy()
	{
		mock_cdn_pack* p = new mock_cdn_pack(m_host); 
		p->m_connect_state = m_connect_state;
		return p;
	}
	virtual void open() { m_connect_state = CONNECTED; }
	virtual void close() { m_connect_state = CONNECT_WAITING; }
	virtual unsigned int get_connect_state() { return m_connect_state; }
	virtual bool is_equal(cdn_pack_base* rp) { return get_host() == rp->get_host();}

public:
	std::string		m_host;
	unsigned int	m_connect_state;
};

class mock_multi_cdn_strategy : public multi_cdn_strategy
{
	friend class MultiCdnStrategyTestCase;
public:
	mock_multi_cdn_strategy(vod_connect_dispatcher * dispatch_ptr, unsigned long bitrate);
	virtual ~mock_multi_cdn_strategy();

	void clear_cdn_pack();

protected:
	virtual void get_query_cdn_result(std::list<std::string>& cdns);
	virtual void get_all_cdns_from_dispacther(std::vector<cdn_pack_base*>& cdns);
 	virtual void get_connected_cdns_from_dispatcher(std::vector<cdn_pack_base*>& cdns);
	virtual void do_dispatch() {}

	cdn_pack_base* convert_to_cdn_pack(cdn_pack_base* pack_ptr);

public:
	std::list<std::string>				m_query_cdn_result;
	std::vector<cdn_pack_base*>			m_mock_cdn_packs;

};

class MyTestCase : public CPPUNIT_NS::TestFixture
{
protected:
	void testMethod();
//////////////////////////////////////////////////////////////////////////

// CPPUNIT_TEST_SUITE(MyTestCast);
public:
	typedef MyTestCase TestFixtureType;

private:
	static const CPPUNIT_NS::TestNamer &getTestNamer__() 
	{
		static CPPUNIT_NS::TestNamer testNamer( std::string("MyTestCase") );
		return testNamer;
	}

public:
	typedef CPPUNIT_NS::TestSuiteBuilderContext<TestFixtureType> TestSuiteBuilderContextType;

	static void addTestsToSuite( CPPUNIT_NS::TestSuiteBuilderContextBase &baseContext )
	{
		TestSuiteBuilderContextType context( baseContext );

// CPPUNIT_TEST(testMethod);
		context.addTest( 
			new CPPUNIT_NS::TestCaller<TestFixtureType>( 
			context.getTestNameFor("testMethod"), 
			&TestFixtureType::testMethod, 
			context.makeFixture() ) );

// CPPUNIT_TEST_SUITE_END;
	}

	static CPPUNIT_NS::TestSuite *suite() 
	{
		const CPPUNIT_NS::TestNamer &namer = getTestNamer__();
		std::auto_ptr<CPPUNIT_NS::TestSuite> suite( 
			new CPPUNIT_NS::TestSuite( namer.getFixtureName() ));
		CPPUNIT_NS::ConcretTestFixtureFactory<TestFixtureType> factory;
		CPPUNIT_NS::TestSuiteBuilderContextBase context( *suite.get(),
			namer,
			factory ); 
		TestFixtureType::addTestsToSuite( context );
		return suite.release();
	}
private:
	typedef int CppUnitDummyTypedefForSemiColonEnding__;
};

class MultiCdnStrategyTestCase : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( MultiCdnStrategyTestCase );
	CPPUNIT_TEST( TestInitState );
	CPPUNIT_TEST( TestStateChange );
	CPPUNIT_TEST( TestInitEventNormal );
	CPPUNIT_TEST( TestInitEventMoreCdnPipe );
	CPPUNIT_TEST( TestInitEventSingleQueryResult );
	CPPUNIT_TEST( TestInitEventQueryError );
	CPPUNIT_TEST( TestOpenCdnNormal );
	CPPUNIT_TEST( TestOpenCdnSingleResult );
	CPPUNIT_TEST( TestOpenCdnNoInit );
	CPPUNIT_TEST( TestScaleStateNormal );
	CPPUNIT_TEST( TestCompareStateNormal );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

protected:
	void TestInitState();			// 初始化状态为 state_uninit
	void TestStateChange();			// 各种状态之间转换的可能性
	
	// 初始化事件到来
	void TestInitEventNormal();	
	void TestInitEventMoreCdnPipe();
	void TestInitEventSingleQueryResult();
	void TestInitEventQueryError();

	// 启动一个cdn
	void TestOpenCdnNormal();
	void TestOpenCdnSingleResult();
	void TestOpenCdnNoInit();

	// 进入单个cdn测速状态，小于码率时加入一个新cdn
	void TestScaleStateNormal();

	// 进入多个cdn比较测速状态
	void TestCompareStateNormal();

	mock_multi_cdn_strategy *					m_multi_cdn_ptr;
};

int run_multi_cdn_strategy_test_case();
