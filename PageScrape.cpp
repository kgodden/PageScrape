// PageScrape.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PageScrape.h"

#include <afxinet.h>

#include <boost/regex.hpp>

#include "getopt.h"
#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace boost;

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

// PageScrape -s"quote.yahoo.com" -r"q?s=A&d=v1" -e"Volume.*</tr.*</td.*</td.*([0-9]{2}.[0-9]{2}).*</td.*Chart"
// PageScrape "www.met.ie" "in Brief.*<font.*>(.*)<"
// PageScrape "weather.yahoo.com/forecast/EIXX0014_f.html" "Today:</b>(.*)<p"

// PageScrape -s"weather.yahoo.com/forecast/SPXX0015_f.html" -e"<HTML>(.*)</HTML>" -e"Currently:.*(<!--.*-->).*([0-9]{2,3}).*<" -b2

// Doesn't really work..
// PageScrape -s"www.wunderground.com/global/stations/03772.html" -e"Observed.*Temperature.*F.*<b>([0-9]{1,}).*C.*Humidity" -v -p

CWinApp theApp;

using namespace std;

struct Param
{
	int requiredBuffer;
	bool verbose;
	bool printBuffers;
	bool ignoreCase;
	bool greedy;
	bool logStream;
	bool multipleMatches;
	string logFile;
	string outputFile;
	string outputFormat;
	string usernamePassword;
	string referrer;

	Param() :
		requiredBuffer(1),
		verbose(false),
		printBuffers(false),
		ignoreCase(false),
		greedy(false),
		logStream(false),
		multipleMatches(false)
	{
	}
};

//-u"file://wotd.log" -e"WOTD=\W+(\w*)\W.*>\W*(\w.*)\W*<" -f"\$1-\$2"

/*
C:\Projects\WebScrape\PageScrape\Debug>pagescrape -u"file://wotd.log" -e"WOTD=\W
+(\w*)\W.*>\W*(\w.*)\W*<" -f"<HTML><H1>\$1</H1><P>\$2</P></HTML>" -o"\temp\it.ht
m"
*/
bool FormatOutput(smatch& r, Param& parms, string& rsResult)
{
	if (parms.outputFormat.empty())
		return false;

	stringstream ss;
	
	int p = 0;
	int last_p = 0;

	string sFormat = parms.outputFormat;
	
	try
	{
		while (p != string::npos && p < sFormat.size())
		{
			p = sFormat.find("\\", p);

			if (p == string::npos || p == sFormat.size() - 1)
			{
				ss << sFormat.substr(last_p, p);
			}
			else
			{
				if (p > last_p)
					ss << sFormat.substr(last_p, p - last_p);

				char c = sFormat[++p];
				
				switch (c)
				{
					case 'n':
						ss << "\n";
						break;
					case 't':
						ss << "\t";
						break;
					case 'r':
						ss << "\r";
						break;
					case '$':
						if (p < sFormat.size() - 1)
						{
							int bufIndex = sFormat[++p] - '0';
							string sBuf = string(r[bufIndex].first, r[bufIndex].second);
							ss << sBuf;
						}
						else
							ss << "\\$";
						break;
				}
				last_p = ++p;
			}
		}
	}
	catch (...)
	{
		cerr << "Error, Exception thrown in FormatOutput()" << endl;	
	}

	rsResult = ss.str();
		
	return true;
}

bool TryToMatch(char *pBuf, const int iLength, const string& rsRegEx, vector<string>& aResults, Param& parms, int& endOfMatchPos)
{
	pBuf[iLength] = '\0';
	endOfMatchPos = 0;

	const regex e(rsRegEx, regbase::normal | (parms.ignoreCase ? regbase::icase : regbase::normal));

	smatch r;

	if (parms.verbose)
		cout << "Performing " << (parms.greedy ? "greedy" : "non-greedy") <<  " search for: " << rsRegEx << endl;

	bool match = regex_search(pBuf, r, e, parms.greedy ? match_default : match_any);

	if (match)
	{
		endOfMatchPos = r.position(0) + r.length(0);

		if (parms.printBuffers)
		{
			for (int i = 0; i < r.size(); i++)
			{
				string sBuffer = std::string(r[i].first, r[i].second); 
				cout << endl << "Buffer" << i << ": " << endl << sBuffer << endl;
			}
		}

		string sResult;

		//	Format output if necessary
		if (parms.outputFormat.empty())
		{
			sResult = std::string(r[parms.requiredBuffer].first, r[parms.requiredBuffer].second);
		}
		else
		{
			if (!FormatOutput(r, parms, sResult))
			{
				cerr << "Error formatting Output.  Output format is - " << endl << parms.outputFormat << endl;
			}
		}

		aResults.push_back(sResult);
	}

	return match;
}

bool ParseUsernamePassword(const string& rsUsernamePassword, string& rsUsername, string& rsPassword)
{
	const regex e("[^:]*:[^:]*");

	smatch r;

	bool match = false;

	try
	{
		match = regex_search(rsUsernamePassword, r, e, match_default);

		if (match)
		{
			rsUsername = std::string(r[1].first, r[1].second); 
			rsPassword = std::string(r[2].first, r[2].second); 
		}
	}
	catch (...)
	{
		cerr << "Error, Exception thrown in ParseUsernamePassword()" << endl;
	}

	return match;
}

bool VisitReferrer(string sUsername, string sPassword, Param& parms)
{

	if (parms.verbose)
		cout << "Visiting Referrer " << parms.referrer << endl;

	regex e_url;

	string referrer;
	string server;

	smatch rReferrer;

	if (regex_match(parms.referrer, rReferrer, regex("^(http://)?([^/]*\\.[^/]*\\.[^/]*)(.*)?", regbase::normal | regbase::icase)))
	{
		server = string(rReferrer[2].first, rReferrer[2].second);
		referrer = string(rReferrer[3].first, rReferrer[3].second);
	}
	else
	{
		cerr << "Error invalid Referrer URL" << endl;
		return false;
	}
	 
	CInternetSession s(NULL, 1, PRE_CONFIG_INTERNET_ACCESS);


	CHttpConnection* pC = NULL;

	try
	{
		pC = s.GetHttpConnection(server.c_str(), INTERNET_INVALID_PORT_NUMBER, sUsername.c_str(), sPassword.c_str());
	}
	catch (CInternetException& e)
	{
		cerr << "Error while connecting to HTTP server=" << server << ", error=" << e.m_dwError << endl;
		return false;
	}

	if (pC == NULL)
	{
		cerr << "Could not connect to HTTP server=" << server << endl;
		return false;
	}

	if (parms.verbose)
	{
		cout << "Connected OK" << endl;
		cout << "Sending request: " << referrer << endl;
	}


	CHttpFile* pF = pC->OpenRequest(CHttpConnection::HTTP_VERB_GET, /*"q?s=A&d=v1"*/ referrer.c_str());

	const TCHAR szHeaders[] =
		_T("Accept: text/*\r\nPragma: no-cache\r\nHost: ");

	CString sHeaders(szHeaders);

	string sDest = server; 

	sHeaders += sDest.c_str();
	sHeaders += "\r\n";

	pF->AddRequestHeaders(sHeaders);

	if (pF == NULL)
	{
		cerr << "Error while sending a request to HTTP server=" << server << endl;
		return false;
	}

	try
	{
		pF->SendRequest();
	}
	catch (CInternetException& e)
	{
		cerr << "Error while sending a request to HTTP server=" << server << ", error=" << e.m_dwError << endl;
		return false;
	}
	catch(...)
	{
		cerr << "Error while sending a request to HTTP server=" << server << endl;
		return false;
	}


	return true;
}


bool ScrapeHTTP(const string& rsServer, const string& rsRequest, const string& rsRegEx, vector<string>& aResults, Param& parms) 
{
	// Parse Username:Password string if not empty.

	string sUsername;
	string sPassword;

	if (!parms.usernamePassword.empty())
	{
		if (!ParseUsernamePassword(parms.usernamePassword, sUsername, sPassword))
		{
			cerr << "Error, invalid username:password string, format must be <username>:<password>" << endl;
			return false;
		}

		if (parms.verbose)
		{
			cout << "Username: " << sUsername << endl;
			cout << "Password: " << sPassword << endl;
		}
	}

	if (!parms.referrer.empty())
	{
		VisitReferrer(sUsername, sPassword, parms);
	}

	CInternetSession s(NULL, 1, PRE_CONFIG_INTERNET_ACCESS);


	CHttpConnection* pC = NULL;

	if (parms.verbose)
		cout << "Connecting to " << rsServer << endl;

	try
	{
		pC = s.GetHttpConnection(rsServer.c_str(), INTERNET_INVALID_PORT_NUMBER, sUsername.c_str(), sPassword.c_str());
	}
	catch (CInternetException& e)
	{
		cerr << "Error while connecting to HTTP server=" << rsServer << ", error=" << e.m_dwError << endl;
		return false;
	}

	if (pC == NULL)
	{
		cerr << "Could not connect to HTTP server=" << rsServer << endl;
		return false;
	}

	if (parms.verbose)
	{
		cout << "Connected OK" << endl;
		cout << "Sending request: " << rsRequest << endl;
	}


	CHttpFile* pF = pC->OpenRequest(CHttpConnection::HTTP_VERB_GET, /*"q?s=A&d=v1"*/ rsRequest.c_str());

	const TCHAR szHeaders[] =
		_T("Accept: text/*\r\nPragma: no-cache\r\nHost: ");

	CString sHeaders(szHeaders);

	string sDest = rsServer; 

	sHeaders += sDest.c_str();
	sHeaders += "\r\n";

	pF->AddRequestHeaders(sHeaders);

	if (pF == NULL)
	{
		cerr << "Error while sending a request to HTTP server=" << rsServer << endl;
		return false;
	}

	try
	{
		pF->SendRequest();
	}
	catch (CInternetException& e)
	{
		cerr << "Error while sending a request to HTTP server=" << rsServer << ", error=" << e.m_dwError << endl;
		return false;
	}
	catch(...)
	{
		cerr << "Error while sending a request to HTTP server=" << rsServer << endl;
		return false;
	}

	int length = 2 * 1024 * 1024;

	char* pBuf = new char[length];

	if (pBuf == NULL)
	{
		cerr << "Error could not allocate memory" << endl;
		return false;
	}

	int iLen = 0;
	int iTotalRead = 0;

	if (parms.verbose)
		cout << "Waiting for response..." << endl;
/*
	try
	{
		do
		{			
			iLen = pF->Read(pBuf + iTotalRead, length - iTotalRead);
		
			if (parms.verbose)
				cout << "Received " << iLen << " bytes" << endl;

			iTotalRead += iLen;

			if (!fMatch)
			{
				fMatch = TryToMatch(pBuf, iTotalRead, rsRegEx, rsResult, parms);
			}
		}
		while ((!fMatch || parms.logStream) && iLen);
	}
	catch(CInternetException& e)
	{
		cerr << "Error Reading page, error=" << e.m_dwError << endl;
		delete [] pBuf;
		return false;
	}
*/

	try
	{
		do
		{			
			iLen = pF->Read(pBuf + iTotalRead, length - iTotalRead);
		
			if (parms.verbose)
				cout << "Received " << iLen << " bytes" << endl;

			iTotalRead += iLen;
		}
		while (iLen);
	}
	catch(CInternetException& e)
	{
		cerr << "Error Reading page, error=" << e.m_dwError << endl;
		delete [] pBuf;
		return false;
	}

	int matchEndPos = 0;
	int startOffset = 0;
	bool fMatch = false;

	string sResult;

	do
	{
		fMatch = TryToMatch(pBuf + startOffset, iTotalRead, rsRegEx, aResults, parms, matchEndPos);
		startOffset += matchEndPos;	
	}
	while (parms.multipleMatches && fMatch);

	if (parms.logStream)
	{
		ofstream os(parms.logFile.c_str());

		if (os)
		{
			if (parms.verbose)
			{
				cout << "Writing stream to " << parms.logFile << endl;
			}

			os << pBuf;
			os.close();
		}
		else
		{
			cerr << "Could not write stream to " << parms.logFile << endl;
		}
	}

	delete pF;
	delete [] pBuf;
	delete pC;

	return !aResults.empty();
}

bool ScrapeFILE(const string& rsServer, const string& rsRequest, const string& rsRegEx, vector<string>& aResults, Param& parms)
{
	if (parms.verbose)
		cout << "Opening file: " << rsServer << endl;
	
	FILE* fp = ::fopen(rsServer.c_str(), "r");
	
	if (fp == NULL)
	{
		cerr << "Could not open " << rsServer << endl;
		return false; 
	}

	int length = 2 * 1024 * 1024;

	char* pBuf = new char[length];

	if (pBuf == NULL)
	{
		cerr << "Error could not allocate memory" << endl;
		::fclose(fp);
		return false;
	}

	int iTotalRead = ::fread(pBuf, sizeof(*pBuf), length - 1, fp);
	pBuf[iTotalRead] = '\0';
	::fclose(fp);

	int matchEndPos = 0;
	int startOffset = 0;

	bool fMatch = false;
	string sResult;

	do
	{
		fMatch = TryToMatch(pBuf + startOffset, iTotalRead, rsRegEx, aResults, parms, matchEndPos);
		startOffset += matchEndPos;	
	}
	while (parms.multipleMatches && fMatch);

	return !aResults.empty();
}

bool SingleScrape(const string& sServer, const string& sRequest, const string& rsRegEx, const bool fFileRequest, Param& parms)
{
	bool fMatch = false;
	vector<string> aResults;

	if (fFileRequest)
	{
		fMatch = ScrapeFILE(sServer, sRequest, rsRegEx, aResults, parms);
	}
	else
	{
		fMatch = ScrapeHTTP(sServer, sRequest, rsRegEx, aResults, parms);
	}
//////////////////////////////////////////////////////

	if (!fMatch)
	{
		cout << "Error Could not match expression" << endl;
	}
	else
	{
		if (parms.verbose)
			cout << "Found match" << endl;

		if (parms.outputFile.empty())
		{
			if (parms.printBuffers)
				cout << endl << "Match:" << endl;

			for (int i = 0; i < aResults.size(); i++)
				cout << aResults[i] << endl;
		}
		else
		{
			ofstream os(parms.outputFile.c_str());

			if (os)
			{
				if (parms.verbose)
					cout << "Writing matched text to " << parms.outputFile << endl; 

				for (int i = 0; i < aResults.size(); i++)
					os << aResults[i] << std::endl;
				
				os.close();
			}
			else
			{
				cerr << "Could not write to " << parms.outputFile << endl;
			}
		}
	}

	return true;
}


int GoGadgetScrape(const string& rsURL, const string& rsRegEx, string& rsResult, Param& parms)
{
	rsResult = "";
	
	// First parse the URL to get the HTTP server name
	// and then the get arguments (if any)

	//const regex e_url("^(.*\\..*\\..*)(/.*)?", regbase::normal | regbase::icase);
	regex e_url;
	bool fFileRequest = false;

	if (regex_match(rsURL, regex("^file://.*", regbase::normal | regbase::icase)))
	{
		e_url = regex("^(file://)?(.*)?", regbase::normal | regbase::icase);
		fFileRequest = true;
	}
	else
	{
		e_url = regex("^(http://)?([^/]*\\.[^/]*\\.[^/]*)(.*)?", regbase::normal | regbase::icase);
	}

	//const regex e_url("^((http|file)://)?([^/]*\\.[^/]*\\.[^/]*)(.*)?", regbase::normal | regbase::icase);

	smatch r_url;
	bool url_match = regex_match(rsURL, r_url, e_url);

	if (!url_match)
	{
		cerr << "Error invalid URL " << endl;
		return -1;
	}
	 
	string sServer = string(r_url[2].first, r_url[2].second);
	string sRequest = string(r_url[3].first, r_url[3].second);

////////////////////////////////////////////////////////////

	bool finishedScraping = false;
	bool fFoundAMatch = false;

	while (!finishedScraping)
	{
		finishedScraping = SingleScrape(sServer, sRequest, rsRegEx, fFileRequest, parms);

		if (!finishedScraping)
			fFoundAMatch = true;	
	}

	return fFoundAMatch ? 0 : -1;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	//bool fVerbose = false;
	//bool fPrintBuffers = false;
	//bool fLogStream = false;
	//bool fIgnoreCase = false;
	//bool fGreedy = false;

	Param parms;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
/////////////////////////////////

		string sURL;
		string sRegEx;

//		int iRequiredBuffer = 1;

		// Register Options
		CGetOpt getOpt(argc,argv,"mgivpu:e:b:o:l:f:a:r:");

		char optionChar;
		while((optionChar = getOpt()) != EOF)
		{
			switch(optionChar)
			{
			case 'u':
				sURL = getOpt.OptArg();
				break;
			case 'e':
				sRegEx = getOpt.OptArg();
				break;
			case 'b':
				parms.requiredBuffer = atoi(getOpt.OptArg());
				break;
			case 'o':
				parms.outputFile = getOpt.OptArg();
				break;
			case 'l':
				parms.logStream = true;
				parms.logFile = getOpt.OptArg();
				break;
			case 'f':
				parms.outputFormat = getOpt.OptArg();
				break;
			case 'a':
				parms.usernamePassword = getOpt.OptArg();
				break;
			case 'r':
				parms.referrer = getOpt.OptArg();
				break;
			case 'v':
				parms.verbose = true;
				break;
			case 'p':
				parms.printBuffers = true;
				break;
			case 'i':
				parms.ignoreCase = true;
				break;
			case 'g':
				parms.greedy = true;
				break;
			case 'm':
				parms.multipleMatches = true;
				break;
			default:
				break;
			}
		}

		if (sURL.empty() || sRegEx.empty())
		{
			cout << "\nScrapes stuff from Web Pages" << endl << endl;
			cout << "pscrape -u<URL> -e<SearchExpr> [-b<BufIndex>] [-o<outputFile>] [-l<logFile>]" << endl;
			cout << "\t[-f<formatString>] [-a<Username:Password>] [-r<Referrer URL>]" << endl;
			cout << "\t[-i] [-v] [-p] [-m]" << endl;
			cout << endl;
			cout << "  -u URL, may include GET parameters, protocol is 'http://' by default," << endl;
			cout << "     if it is passed as 'file://' will match against the specified file." << endl;
			cout << "  -e Regular Expression to be matched with received data." << endl;
			cout << "  -b Index of RegEx buffer to be returned (defaults to 1)." << endl;
			cout << "  -g Perform greedy search (default is non-greedy)." << endl;
			cout << "  -i Search ignoring case." << endl;
			cout << "  -o Output to specified file rather than stdout." << endl;
			cout << "  -v Verbose mode, outputs progress messages." << endl;
			cout << "  -p Print Buffers, prints all of the matched RegEx Buffers." << endl;
			cout << "  -l Log received HTML to specified log file." << endl;
			cout << "  -f Format output as specified by a format string." << endl;
			cout << "  -a Specify Username and Password for HTTP Authentication," << endl;
			cout << "     Username and Password must be separated by : (a colon)" << endl;
			cout << "  -r Specify Referrer URL, PageScrape will visit this URL before scraping" << endl;
			cout << "     from the target URL picking up cookies on the way." << endl;
			cout << "  -m Search for multiple matches, the result of each match" << endl;
			cout << "     is output on a separate line." << endl << endl;
			cout << "PageScrape v1.1.01 (c) 2005 www.webscrape.com" << endl;

			return -1;
		}

		string sResult;

		return GoGadgetScrape(sURL, sRegEx, sResult, parms);
	}

	return nRetCode;
}
