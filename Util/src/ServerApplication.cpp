//
// ServerApplication.cpp
//
// $Id: //poco/1.3/Util/src/ServerApplication.cpp#2 $
//
// Library: Util
// Package: Application
// Module:  ServerApplication
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
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


#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Exception.h"
#include "Poco/Process.h"
#include "Poco/NumberFormatter.h"
#include "Poco/NamedEvent.h"
#include "Poco/Logger.h"
#if defined(POCO_OS_FAMILY_UNIX)
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#elif defined(POCO_OS_FAMILY_WINDOWS)
#include "Poco/Util/WinService.h"
#include <windows.h>
#include <string.h>
#endif
#if defined(POCO_WIN32_UTF8)
#include "Poco/UnicodeConverter.h"
#endif


using Poco::NamedEvent;
using Poco::Process;
using Poco::NumberFormatter;
using Poco::Exception;
using Poco::SystemException;


namespace Poco {
namespace Util {


#if defined(POCO_OS_FAMILY_WINDOWS)
Poco::Event     ServerApplication::_terminated;
SERVICE_STATUS        ServerApplication::_serviceStatus; 
SERVICE_STATUS_HANDLE ServerApplication::_serviceStatusHandle = 0; 
#endif


ServerApplication::ServerApplication()
{
#if defined(POCO_OS_FAMILY_WINDOWS)
	_action = SRV_RUN;
	memset(&_serviceStatus, 0, sizeof(_serviceStatus));
#endif
}


ServerApplication::~ServerApplication()
{
}


bool ServerApplication::isInteractive() const
{
	bool runsInBackground = config().getBool("application.runAsDaemon", false) || config().getBool("application.runAsService", false);
	return !runsInBackground;
}


int ServerApplication::run()
{
	return Application::run();
}


void ServerApplication::terminate()
{
	Process::requestTermination(Process::id());
}


#if defined(POCO_OS_FAMILY_WINDOWS)


//
// Windows specific code
//
BOOL ServerApplication::ConsoleCtrlHandler(DWORD ctrlType)
{
	switch (ctrlType) 
	{ 
	case CTRL_C_EVENT: 
	case CTRL_CLOSE_EVENT: 
	case CTRL_BREAK_EVENT:
		terminate();
		return _terminated.tryWait(10000) ? TRUE : FALSE;
	default: 
		return FALSE; 
	}
}


void ServerApplication::ServiceControlHandler(DWORD control)
{
	switch (control) 
	{ 
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		terminate();
		_serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		break;
	case SERVICE_CONTROL_INTERROGATE: 
		break; 
	} 
	SetServiceStatus(_serviceStatusHandle,  &_serviceStatus);
}


void ServerApplication::ServiceMain(DWORD argc, LPTSTR* argv)
{
	ServerApplication& app = static_cast<ServerApplication&>(Application::instance());

#if defined(POCO_WIN32_UTF8)
	_serviceStatusHandle = RegisterServiceCtrlHandlerW(L"", ServiceControlHandler);
#else
	_serviceStatusHandle = RegisterServiceCtrlHandler("", ServiceControlHandler);
#endif
	if (!_serviceStatusHandle)
		throw SystemException("cannot register service control handler");

	_serviceStatus.dwServiceType             = SERVICE_WIN32; 
	_serviceStatus.dwCurrentState            = SERVICE_START_PENDING; 
	_serviceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	_serviceStatus.dwWin32ExitCode           = 0; 
	_serviceStatus.dwServiceSpecificExitCode = 0; 
	_serviceStatus.dwCheckPoint              = 0; 
	_serviceStatus.dwWaitHint                = 0; 
	SetServiceStatus(_serviceStatusHandle, &_serviceStatus);

	try
	{
#if defined(POCO_WIN32_UTF8)
		std::vector<std::string> args;
		for (DWORD i = 0; i < argc; ++i)
		{
			std::string arg;
			Poco::UnicodeConverter::toUTF8(argv[i], arg);
			args.push_back(arg);
		}
		app.init(args);
#else
		app.init(argc, argv);
#endif
		_serviceStatus.dwCurrentState = SERVICE_RUNNING; 
		SetServiceStatus(_serviceStatusHandle, &_serviceStatus);
		int rc = app.run();
		_serviceStatus.dwWin32ExitCode           = rc ? ERROR_SERVICE_SPECIFIC_ERROR : 0;
		_serviceStatus.dwServiceSpecificExitCode = rc; 
	}
	catch (Exception& exc)
	{
		app.logger().log(exc);
		_serviceStatus.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
		_serviceStatus.dwServiceSpecificExitCode = EXIT_CONFIG; 
	}
	catch (...)
	{
		app.logger().error("fatal error - aborting");
		_serviceStatus.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
		_serviceStatus.dwServiceSpecificExitCode = EXIT_SOFTWARE; 
	}
	try
	{
		app.uninitialize();
	}
	catch (...)
	{
	}
	_serviceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(_serviceStatusHandle, &_serviceStatus);
}


void ServerApplication::waitForTerminationRequest()
{
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	std::string evName("POCOTRM");
	evName.append(NumberFormatter::formatHex(Process::id(), 8));
	NamedEvent ev(evName);
	ev.wait();
	_terminated.set();
}


int ServerApplication::run(int argc, char** argv)
{
	if (!hasConsole() && isService())
	{
		config().setBool("application.runAsService", true);
		return 0;
	}
	else 
	{
		int rc = EXIT_OK;
		try
		{
			init(argc, argv);
			switch (_action)
			{
			case SRV_REGISTER:
				registerService();
				rc = EXIT_OK;
				break;
			case SRV_UNREGISTER:
				unregisterService();
				rc = EXIT_OK;
				break;
			default:
				rc = run();
				uninitialize();
			}
		}
		catch (Exception& exc)
		{
			logger().log(exc);
			rc = EXIT_SOFTWARE;
		}
		return rc;
	}
}


#if defined(POCO_WIN32_UTF8)
int ServerApplication::run(int argc, wchar_t** argv)
{
	if (!hasConsole() && isService())
	{
		config().setBool("application.runAsService", true);
		return 0;
	}
	else 
	{
		int rc = EXIT_OK;
		try
		{
			init(argc, argv);
			switch (_action)
			{
			case SRV_REGISTER:
				registerService();
				rc = EXIT_OK;
				break;
			case SRV_UNREGISTER:
				unregisterService();
				rc = EXIT_OK;
				break;
			default:
				rc = run();
				uninitialize();
			}
		}
		catch (Exception& exc)
		{
			logger().log(exc);
			rc = EXIT_SOFTWARE;
		}
		return rc;
	}
}
#endif


bool ServerApplication::isService()
{
	SERVICE_TABLE_ENTRY svcDispatchTable[2];
#if defined(POCO_WIN32_UTF8)
	svcDispatchTable[0].lpServiceName = L"";
#else
	svcDispatchTable[0].lpServiceName = "";
#endif
	svcDispatchTable[0].lpServiceProc = ServiceMain;
	svcDispatchTable[1].lpServiceName = NULL;
	svcDispatchTable[1].lpServiceProc = NULL; 

	return StartServiceCtrlDispatcher(svcDispatchTable) != 0; 
}


bool ServerApplication::hasConsole()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	return  hStdOut != INVALID_HANDLE_VALUE && hStdOut != NULL;
}


void ServerApplication::registerService()
{
	std::string name = config().getString("application.baseName");
	std::string path = config().getString("application.path");
	
	WinService service(name);
	if (_displayName.empty())
		service.registerService(path);
	else
		service.registerService(path, _displayName);
	logger().information("The application has been successfully registered as a service");
}


void ServerApplication::unregisterService()
{
	std::string name = config().getString("application.baseName");
	
	WinService service(name);
	service.unregisterService();
	logger().information("The service has been successfully unregistered");
}


void ServerApplication::defineOptions(OptionSet& options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("registerService", "", "register application as a service")
			.required(false)
			.repeatable(false));

	options.addOption(
		Option("unregisterService", "", "unregister application as a service")
			.required(false)
			.repeatable(false));

	options.addOption(
		Option("displayName", "", "specify a display name for the service (only with /registerService)")
			.required(false)
			.repeatable(false)
			.argument("name"));
}


void ServerApplication::handleOption(const std::string& name, const std::string& value)
{
	if (name == "registerService")
		_action = SRV_REGISTER;
	else if (name == "unregisterService")
		_action = SRV_UNREGISTER;
	else if (name == "displayName")
		_displayName = value;
	else
		Application::handleOption(name, value);
}


#elif defined(POCO_OS_FAMILY_UNIX)


//
// Unix specific code
//
void ServerApplication::waitForTerminationRequest()
{
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGQUIT);
	sigaddset(&sset, SIGTERM);
	sigprocmask(SIG_BLOCK, &sset, NULL);
	int sig;
	sigwait(&sset, &sig);
}


int ServerApplication::run(int argc, char** argv)
{
	bool runAsDaemon = isDaemon(argc, argv);
	if (runAsDaemon)
	{
		beDaemon();
	}
	try
	{
		init(argc, argv);
		if (runAsDaemon)
		{
			chdir("/");
		}
	}
	catch (Exception& exc)
	{
		logger().log(exc);
		return EXIT_CONFIG;
	}
	int rc = run();
	try
	{
		uninitialize();
	}
	catch (Exception& exc)
	{
		logger().log(exc);
		rc = EXIT_CONFIG;
	}
	return rc;
}


bool ServerApplication::isDaemon(int argc, char** argv)
{
	std::string option("--daemon");
	for (int i = 1; i < argc; ++i)
	{
		if (option == argv[i])
			return true;
	}
	return false;
}


void ServerApplication::beDaemon()
{
	pid_t pid;
	if ((pid = fork()) < 0)
		throw SystemException("cannot fork daemon process");
	else if (pid != 0)
		exit(0);
	
	setsid();
	umask(0);
	close(0);
	close(1);
	close(2);
}


void ServerApplication::defineOptions(OptionSet& options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("daemon", "", "run application as a daemon")
			.required(false)
			.repeatable(false));
}


void ServerApplication::handleOption(const std::string& name, const std::string& value)
{
	if (name == "daemon")
	{
		config().setBool("application.runAsDaemon", true);
	}
	else Application::handleOption(name, value);
}


#elif defined(POCO_OS_FAMILY_VMS)


//
// VMS specific code
//
namespace
{
	static void handleSignal(int sig)
	{
		ServerApplication::terminate();
	}
}


void ServerApplication::waitForTerminationRequest()
{
	struct sigaction handler;
	handler.sa_handler = handleSignal;
	handler.sa_flags   = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGQUIT, &handler, NULL);                                       

	long ctrlY = LIB$M_CLI_CTRLY;
	unsigned short ioChan;
	$DESCRIPTOR(ttDsc, "TT:");

	lib$disable_ctrl(&ctrlY);
	sys$assign(&ttDsc, &ioChan, 0, 0);
	sys$qiow(0, ioChan, IO$_SETMODE | IO$M_CTRLYAST, 0, 0, 0, terminate, 0, 0, 0, 0, 0);
	sys$qiow(0, ioChan, IO$_SETMODE | IO$M_CTRLCAST, 0, 0, 0, terminate, 0, 0, 0, 0, 0);

	std::string evName("POCOTRM");
	evName.append(NumberFormatter::formatHex(Process::id(), 8));
	NamedEvent ev(evName);
	try
	{
		ev.wait();
    }
	catch (...)
	{
		// CTRL-C will cause an exception to be raised
	}
	sys$dassgn(ioChan);
	lib$enable_ctrl(&ctrlY);
}


int ServerApplication::run(int argc, char** argv)
{
	try
	{
		init(argc, argv);
	}
	catch (Exception& exc)
	{
		logger().log(exc);
		return EXIT_CONFIG;
	}
	int rc = run();
	try
	{
		uninitialize();
	}
	catch (Exception& exc)
	{
		logger().log(exc);
		rc = EXIT_CONFIG;
	}
	return rc;
}


void ServerApplication::defineOptions(OptionSet& options)
{
	Application::defineOptions(options);
}


void ServerApplication::handleOption(const std::string& name, const std::string& value)
{
	Application::handleOption(name, value);
}


#endif


} } // namespace Poco::Util
