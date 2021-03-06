//
// PBKDF2EngineTest.cpp
//
// $Id: //poco/1.4/Foundation/testsuite/src/PBKDF2EngineTest.cpp#1 $
//
// Copyright (c) 2014, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "PBKDF2EngineTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/PBKDF2Engine.h"
#include "Poco/HMACEngine.h"
#include "Poco/SHA1Engine.h"


using Poco::PBKDF2Engine;
using Poco::HMACEngine;
using Poco::SHA1Engine;
using Poco::DigestEngine;


PBKDF2EngineTest::PBKDF2EngineTest(const std::string& name): CppUnit::TestCase(name)
{
}


PBKDF2EngineTest::~PBKDF2EngineTest()
{
}


void PBKDF2EngineTest::testPBKDF2a()
{
	// test vector 1 from RFC 6070
	
	std::string p("password");
	std::string s("salt");
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 1, 20);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "0c60c80f961f0e71f3a9b524af6012062fe037a6"); 
}


void PBKDF2EngineTest::testPBKDF2b()
{
	// test vector 2 from RFC 6070
	
	std::string p("password");
	std::string s("salt");
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 2, 20);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957");
}


void PBKDF2EngineTest::testPBKDF2c()
{
	// test vector 3 from RFC 6070
	
	std::string p("password");
	std::string s("salt");
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 4096, 20);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "4b007901b765489abead49d926f721d065a429c1");
}


void PBKDF2EngineTest::testPBKDF2d()
{
	// test vector 4 from RFC 6070
	
	std::string p("password");
	std::string s("salt");
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 16777216, 20);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "eefe3d61cd4da4e4e9945b3d6ba2158c2634e984");
}


void PBKDF2EngineTest::testPBKDF2e()
{
	// test vector 5 from RFC 6070
	
	std::string p("passwordPASSWORDpassword");
	std::string s("saltSALTsaltSALTsaltSALTsaltSALTsalt");
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 4096, 25);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038");
}


void PBKDF2EngineTest::testPBKDF2f()
{
	// test vector 6 from RFC 6070
	
	std::string p("pass\0word", 9);
	std::string s("sa\0lt", 5);
	PBKDF2Engine<HMACEngine<SHA1Engine> > pbkdf2(s, 4096, 16);
	pbkdf2.update(p);
	std::string dk = DigestEngine::digestToHex(pbkdf2.digest());
	assert (dk == "56fa6aa75548099dcc37d7f03425e0c3");
}


void PBKDF2EngineTest::setUp()
{
}


void PBKDF2EngineTest::tearDown()
{
}


CppUnit::Test* PBKDF2EngineTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("PBKDF2EngineTest");

	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2a);
	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2b);
	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2c);
	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2d);
	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2e);
	CppUnit_addTest(pSuite, PBKDF2EngineTest, testPBKDF2f);

	return pSuite;
}
