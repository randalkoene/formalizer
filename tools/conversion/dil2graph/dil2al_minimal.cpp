// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This file contains a minimal subset of variables and functions defined in dil2al/dil2al.cc.
 * These are merely meant to make basic initialization and utilization possible with the dil2al
 * DIL Hierarchy v1.x files. An alternative would be to link with the dil2al.o object code in
 * the Makefile. (This approach is used for other functions, such as functions in dil2al/utilities.cc.)
 */

//#define USE_COMPILEDPING

// std
#include <sys/wait.h>

// core
#include "error.hpp"
#include "standard.hpp"

// dil2al compatibility
#include "dil2al.hh"

// local
#include "dil2al_minimal.hpp"


String runnablename;
String runningcommand("dil2al:(unspecified)"); // Which command request is running, prepended with runnablename for use in UI_ output.
bool noX = false;
bool stricttimestamps = true; // This makes functions like time_stamp_time() exit for bad time stamps.
String homedir = DEFAULTHOMEDIR;
String basedir = RELDIRHTML;
String listfile = DILMAIN;
String idfile = DILBYIDFILE;
String outputlog = DEFAULTOUTPUTLOG; // File that receives a HTML formatted copy of program output for perusal.
String lockidfile;
String crcidfile;
String cacheidfile;
String showmetricsflagfile = SHOWMETRICSFLAGFILE;
String tasklog = TASKLOGFILE;
String taskloghead = "";
String firstcallpagetemplate = FIRSTCALLPAGETEMPLATE;
String firstcallpagetarget = FIRSTCALLPAGETARGET;
String paperplansfile = PAPERPLANSFILE;
String poolfile = POOLFILE;
String ooplannerfile = OOPLANNERFILE;
String figuresdirectory = FIGURESDIRECTORY;
String dilref = DILREFFILE;
String normalskipexcludesfile = NORMALSKIPEXCLUDESFILE;
String tareadfile = TAREADFILE;
bool calledbyforminput = false;
int followlinks = 0;
bool searchprogressmeter = false;
bool immediatesearchoutput = true;
bool remotesearch = true;
String bibindexfilename;
String bibindexfile = BIBINDEXFILE;
String articleheadtemplate = ARTICLEHEADTEMPLATE;
String articletailtemplate = ARTICLETAILTEMPLATE;
String templatearticlesubsection = TEMPLATEARTICLESUBSECTION;
String templatearticlesubsubsection = TEMPLATEARTICLESUBSUBSECTION;
ifstream dinfile;
istream * din; // data input (default cin)
ofstream eoutfile;
ostream * eout; // error output (default cerr)
ostream * vout; // verbouse output (default cout)
char projid = 1; // randomized using the current time for use in UI_ functions
UI_Info * info = new UI_Info_Classic(&cout,true);
UI_Info * warn = new UI_Info_Classic(&cerr,true);
UI_Info * error = new UI_Info_Classic(&cerr,false);
UI_Joined_Yad * ui_joined_yad = NULL; // may be used with UI_Info_Yad
UI_Entry * input = new UI_Entry_Classic(&cout);
UI_Confirm * confirm = new UI_Confirm_Classic(&cout);
UI_Options * options = new UI_Options_Classic(&cout);
UI_Progress * progress = new UI_Progress_Classic(&cout);
int ui_info_warn_timeout = 30;
bool showuicounter = false;
int numDILs;
Topical_DIL_Files_Root dil;
bool verbose = true;
bool extra_verbose = true; // normally false
bool suggestnameref = false;
bool askprocesstype = false;
bool offercreate = false;
bool askALDILref = false;
bool showflag = true;
bool isdaemon = false;
bool useansi = true;
bool askextractconcat = true;
bool simplifyuserrequests = false;
bool usequickloadcache = false; // use binary cache files for quick loading
bool immediatecachewriteback = false; // immediately write changes back to the cache
bool formguaranteestdinout = false; // guarantee that stdin & stdout are used when called from FORM
time_t emulatedtime = 0; // possible alternative current time (0=use actual time)
String curdate, curtime;
String owner = OWNER;
int timechunksize = DEFAULTCHUNKSIZE;
int timechunkslack = DEFAULTCHUNKSLACK;
int timechunkoverstrategy = TCS_ASK; // new time chunk decision strategy when over timechunksize+timechunkslack
int timechunkunderstrategy = TCS_ASK; // new time chunk decision strategy when under timechunksize
int sectionsize = DEFAULTSECTIONSIZE;
int sectionstrategy = TSS_ASK; // new TL section decision strategy
long generatealtcs = DEFAULTALTCS; // number of AL TCs to generate
time_t generatealmaxt = 0; // maximum date to which to general an AL
int alfocused = DEFAULTALFOCUSED; // number of AL TCs to show in the shorter listing
int alfocusedCALexcerptlength = DEFAULTALFOCUSEDCALEXCERPTLENGTH; // character columns of TCs in Cal._Day
int alfocusedCALdays = DEFAULTALFOCUSEDCALDAYS; // number of days to show in top-calendar format
int alfocusedCALcolumns = DEFAULTALFOCUSEDCALCOLUMNS; // number of columns to squeeze top-calendar into
int alfocusedexcerptlength = DEFAULTALFOCUSEDEXCERPTLENGTH; // character columns of TCs in AL list
int alwide = DEFAULTALWIDE; // number of AL TCs to show in the longer listing
int algenregular = DEFAULTALGENREGULAR; // generic number of hours in a regular AL-day
int algenall = DEFAULTALGENALL; // generic number of hours in an AL-day for an AL with all DIL-entries
float alepshours = DEFAULTALEPSHOURS; // assumed available hours per day for EPS scheduling method
bool alautoexpand = true; // automatically expand number of hours per day as necessary
bool alshowcumulativereq = false; // show cumulative time required
int alcumtimereqexcerptlength = DEFAULTALCUMTIMEREQEXCERPTLENGTH; // length of DIL entry text excerpt in row of Cumulative Time Required form
int alCRTlimit = 0; // test used to determine if subsequent entries should be processed for the Cumulative Time Required form
int alCTRdays = 0; // a number of days limits for the CTR AL page (used if alCTRlimit is set to CTRDAYS), 0 is no limit
int alCRTmultiplier = 3; // multiplier to use to have a safe map size for available time chunks
time_t alCRTepsgroupoffset = 2*60; // offset to add to suggested EPS group variable target dates to preserve grouping
bool alCTRGoogleCalendar = false; // create output for the Google calendar and similar calendar formats
float alCTRGoogleCalendarvaluationcriterion = 3.0;
float alCTRGoogleCalendartimecriterion = 5.0;
time_t alCTRGoogleCalendardaysextent = 0;
time_t alCTRGCtimezoneadjust = 0;
bool ctrshowdaycumulative = false; // if true then show a column with the cumulative time for the day
bool ctrshowall = false;
int alperiodictasksincludedayslimit = -1; // if positive, limit the number of days for which periodic tasks are included in AL generation
bool periodicautoupdate = false; // automatically update completion ratios and target dates of periodic tasks
bool periodicadvancebyskip = true; // try periodicskipone before periodicautoaupdate to advance periodic target dates
bool rapidscheduleupdating = true; // use RSU instead of EPS options
bool targetdatepreferences = true; // apply target date preferences in the RSU method
bool prioritypreferences = false; // separate end-of-day scheduling proposals based on priority rating
time_t dolaterendofday = DEFAULTDOLATERENDOFDAY; // proposed TD time later in day with prioritypreferences
time_t doearlierendofday = DEFAULTDOEARLIERENDOFDAY; // proposed TD time earlier in day with prioritypreferences
bool weekdayworkendpreferences = false; // specific days of the week have preffered target times
time_t wdworkendprefs[7] = {-1,-1,-1,-1,-1,-1,-1}; // preferred end of work day for each day of the week starting with Sunday
time_t alworkdaystart = DEFAULTALWORKDAYSTART; // standard start time of work on a regular work day (seconds since midnight)
int alcurdayworkedstrategy = ACW_TIME; // how to estimate the time already worked during the current day
time_t alworked = -1; // time already worked during current day
int alautoupdate = AAU_YES; // automatically update AL and completion ratios
int alautoupdate_TCexceeded = AAU_NO; // automatically update AL and completion rations if the Task Chunk size was significantly exceeded
time_t alsyncslack = DEFAULTALSYNCSLACK; // allowable desynchronization in seconds
int aldaydistfunc = ALDAYDISTFUNCLINEAR; // AL Day distribution function
float aldaydistslope = DEFAULTALDAYDISTSLOPE; // slope of AL Day distribution
// Note: Using the linear distribution, 0.9 implies that the
// last of 10 days has a 5.26 times lower probability of being
// selected than the first day. A slope of 0.5 would imply
// a relative probability p(first)/p(last)=1.82.
// Using a squared distribution, a slope of 1.0 implies
// p(first)/p(last)=5.26, and a slope of 0.8 implies
// p(first)/p(last)=2.84.
// (See <A HREF="disttest.m">~/src/dil2al/disttest.m</A> for more possibilities.)
bool tltrackperformance = false; // track performance data during task chunk work (statistics may be derived from these for system-results.m)
bool reversehierarchy = false; // branch to superiors instead of dependencies
bool hierarchyaddnonobjectives = false; // add some information about non-Objectives when focusing on Objectives
bool dotgraph = false; // hierarchy output in the dot language of GraphViz
int maxdotgraphwidth = 10; // maximum width that should be displayed in GraphViz output
bool hierarchyplanparse = false; // put in some additional processing to display more PLAN entry information
bool hierarchysorting = false; // sort entries at each hierarchical level
int hierarchymaxdepth = DEFAULTHIERARCHYMAXDEPTH; // default depth of hieararchy (note that FORM input has an additional variable with which to specify this)
int hierarchyexcerptlength = DEFAULTHIERARCHYEXCERPTLENGTH; // length of DIL entry text excerpt shown in a DIL hierarchy
String hierarchylabel; // only include DIL entries with a specific @label:LABEL@
String updatenoteineditorcmd = DEFAULTUNIECMD;
int updatenoteineditor = UNIE_ASK; // update note destinations in open editors
String atcommand = DEFAULTATCOMMAND; // comand used to schedule an at-job
String atcontroller = DEFAULTATCONTROLLER; // at-job scheduled controller command
String htmlforminterface = DEFAULT_HTMLFORM_INTERFACE; // interface to dil2al for HTML form data
String htmlformmethod = DEFAULT_HTMLFORM_METHOD; // HTML form submission method
String editor = DEFAULTEDITOR;
String browser = DEFAULTBROWSER;
String taskhistorypage = DEFAULTTASKHISTORYPAGE;
String taskhistorycall = DEFAULTTASKHISTORYCALL;
String metricsdayspage = DEFAULTMETRICSDAYSPAGE;
String mainmetricscategoriesfile = DEFAULTMAINMETRICSCATEGORIESFILE;
String metricsmapfile = DEFAULTMETRICSMAPFILE;
String systemselfevaluationdata = DEFAULTSYSTEMSELFEVALUATIONDATA;
String metricsrcreproduction = DEFAULTMETRICSRCREPRODUCTION;
time_t metricsearliestday = DATEINSECONDS19990101;
bool metrics_are_ratios = true;
bool labeled_metric_strict_invalidate = true;
String nexttaskbrowserpage = homedir+'/'+basedir+DEFAULTNEXTTASKBROWSERPAGE;
String flagcmd = DEFAULTFLAGCMD;
String notetmpfile = NOTETMPFILE;
String tlentrytmpfile = TLENTRYTMPFILE;
String configfile = CONFIGFILE;
String tltrackperformancefile = TLTRACKPERFORMANCEFILE;
String dilpresetfile = DILPRESETFILE;
String virtualoverlapsfile = VIRTUALOVERLAPSFILE;
String exactconflictsfile = EXACTCONFLICTSFILE;
String finnotesfile = FINNOTESFILE;
String calendarfile = CALENDARFILE;
StringList quicknotedst;
StringList quicknotedsttitle;
StringList quicknotedescr;
int quicknotedstnum = 0;
StringList indexkeys;
int indexkeysnum = 0;
String newTLID; // remembers newly created TL entry ID
bool chunkcreated; // remembers if a new chunk was created
time_t ImmediateTD=0;
time_t UrgentTD=0;
time_t NearTermTD=0;
#ifdef LOOP_PROTECT
Detailed_Items_List * looplist = NULL; // list of DIL entries for which loops were detected in the DIL hierarchy
#endif

// These can be used as default values for confirmation functions. Having
// them defined here in one place means being able to change them all at
// once by changing them here.
const char default_defoption[] = "Yes";
const char default_nondefoption[] = "No";

void rcmd_set(const char * cmdlabel) {
  // Use this to modify runningcommand so that any additional
  // actions that should happen when the running command is
  // updated can take place.

  runningcommand = runnablename+cmdlabel;
  Output_Log_Append("Command request: "+runningcommand+'\n');
}

String & rcmd_get() {
  return runningcommand;
}

void Emulated_Time_Check() {
  // A test to prevent unintended runs with an emulated time greater than current time.
  if (Time(NULL)>time(NULL)) {
    // ** Can add an override parameter check here if this test is undesired in a specific circumstance.
    if (confirmation("Emulated Time > actual current time. Proceed anyway? (p/N) ",'p',"No","Proceed")) {
      ERROR << "dil2al: Exiting at Emulated_Time_Check().\n";
      Exit_Now(1);
    }
  }
  // Added 20181005.
}

time_t work_end_preferences(String westr) { // function used in Set_Parameter()
  long a = atol((const char *) westr);
  weekdayworkendpreferences = true;
  return (time_t) (((a/100)*60)+(a%100))*60;
}

void Set_Parameter(char * lbuf) {
	String confdefs[4];
	// notetarget definitions
	if (substring_from_line("^[ 	]*notetarget[ 	]*=[ 	]*([^ 	]+)[ 	]*([^:]+)(.*)$",4,confdefs,lbuf)) {
		if (confdefs[1][0]!='/') confdefs[1].prepend(homedir);
		quicknotedst[quicknotedstnum]=confdefs[1];
		quicknotedsttitle[quicknotedstnum]=confdefs[2];
		quicknotedescr[quicknotedstnum]=confdefs[3];
		quicknotedstnum++;
	}
	// indexkey definitions
	if (substring_from_line("^[ 	]*indexkey[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		indexkeys[indexkeysnum]=confdefs[1];
		indexkeysnum++;
	}
	// preferred editor
	if (substring_from_line("^[ 	]*editor[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) editor=confdefs[1];
	// preferred browser
	if (substring_from_line("^[ 	]*browser[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) browser=confdefs[1];
	// preferred output log
	if (substring_from_line("^[ 	]*outputlog[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) outputlog=confdefs[1];
	// default time chunk size
	if (substring_from_line("^[ 	]*timechunksize[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) timechunksize=atoi((const char *) confdefs[1]);
	// default time chunk slack
	if (substring_from_line("^[ 	]*timechunkslack[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) timechunkslack=atoi((const char *) confdefs[1]);
	// UI_Info derived class to use for info output
	if (substring_from_line("^[ 	]*UI_Info[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete info; info = new UI_Info_Classic(vout,false); }
	  else if (confdefs[1]=="UI_YAD") { delete info; info = new UI_Info_Yad(homedir+INFOICON_LARGE,true); }
#ifdef UI_JOINED_YAD_LIST_STRINGLISTS
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete info; info = new UI_Info_Yad(homedir+INFOICON_SMALL,true,ui_joined_info); }
#else
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete info; info = new UI_Info_Yad(homedir+INFOICON_LARGE,true,ui_joined_info); }
#endif
	  else { delete info; info = new UI_Info_External(confdefs[1],true); }
	}
	// UI_Info derived class to use for error output
	if (substring_from_line("^[ 	]*UI_Error[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete error; error = new UI_Info_Classic(eout,false); }
	  else if (confdefs[1]=="UI_YAD") { delete error; error = new UI_Info_Yad(homedir+ERRORICON_LARGE,false); }
#ifdef UI_JOINED_YAD_LIST_STRINGLISTS
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete error; error = new UI_Info_Yad(homedir+ERRORICON_SMALL,false,ui_joined_error); }
#else
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete error; error = new UI_Info_Yad(homedir+ERRORICON_LARGE,false,ui_joined_error); }
#endif
	  else { delete error; error = new UI_Info_External(confdefs[1],false); }
	}
	// UI_Info derived class to use for warn output
	if (substring_from_line("^[ 	]*UI_Warn[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete warn; warn = new UI_Info_Classic(vout,true); }
	  else if (confdefs[1]=="UI_YAD") { delete warn; warn = new UI_Info_Yad(homedir+WARNICON_LARGE,true); }
#ifdef UI_JOINED_YAD_LIST_STRINGLISTS
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete warn; warn = new UI_Info_Yad(homedir+WARNICON_SMALL,true,ui_joined_warn); }
#else
	  else if (confdefs[1]=="UI_YAD_JOINED") { delete warn; warn = new UI_Info_Yad(homedir+WARNICON_LARGE,true,ui_joined_warn); }
#endif
	  else { delete warn; warn = new UI_Info_External(confdefs[1],true); }
	}
	// UI_Entry derived class to read input strings
	if (substring_from_line("^[ 	]*UI_Entry[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete input; input = new UI_Entry_Classic(vout); }
	  else if (confdefs[1]=="UI_YAD") { delete input; input = new UI_Entry_Yad(); }
	  else { delete input; input = new UI_Entry_External(confdefs[1]); }
	}
	// UI_Confirm derived class to use for confirmations
	if (substring_from_line("^[ 	]*UI_Confirm[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete confirm; confirm = new UI_Confirm_Classic(vout); }
	  else if (confdefs[1]=="UI_YAD") { delete confirm; confirm = new UI_Confirm_Yad(); }
	  else { delete confirm; confirm = new UI_Confirm_External(confdefs[1]); }
	}
	// UI_Options derived class to use for confirmations
	if (substring_from_line("^[ 	]*UI_Options[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete options; options = new UI_Options_Classic(vout); }
	  else if (confdefs[1]=="UI_YAD") { delete options; options = new UI_Options_Yad(); }
	  else { delete options; options = new UI_Options_External(confdefs[1]); }
	}
	// UI_Progress derived class to use for progress indicators
	if (substring_from_line("^[ 	]*UI_Progress[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  if (confdefs[1]=="UI_CLASSIC") { delete progress; progress = new UI_Progress_Classic(vout); }
	  else if (confdefs[1]=="UI_YAD") { delete progress; progress = new UI_Progress_Yad(); }
	  else { delete progress; progress = new UI_Progress_External(confdefs[1]); }
	}
	// new decision strategy when time chunk is over timechunksize+timechunkslack
	if (substring_from_line("^[ 	]*timechunkoverstrategy[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="TCS_CURRENT") timechunkoverstrategy=TCS_CURRENT;
		else if (confdefs[1]=="TCS_NEW") timechunkoverstrategy=TCS_NEW;
		else if (confdefs[1]=="TCS_ASK") timechunkoverstrategy=TCS_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==TCS_CURRENT) || (q==TCS_NEW) || (q==TCS_ASK)) timechunkoverstrategy=q;
		}
	}
	// new decision strategy when time chunk is under timechunksize
	if (substring_from_line("^[ 	]*timechunkunderstrategy[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="TCS_CURRENT") timechunkunderstrategy=TCS_CURRENT;
		else if (confdefs[1]=="TCS_NEW") timechunkunderstrategy=TCS_NEW;
		else if (confdefs[1]=="TCS_ASK") timechunkunderstrategy=TCS_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==TCS_CURRENT) || (q==TCS_NEW) || (q==TCS_ASK)) timechunkunderstrategy=q;
		}
	}
	// new TL section decision strategy
	if (substring_from_line("^[ 	]*sectionstrategy[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="TSS_CURRENT") sectionstrategy=TSS_CURRENT;
		else if (confdefs[1]=="TSS_NEW") sectionstrategy=TSS_NEW;
		else if (confdefs[1]=="TSS_ASK") sectionstrategy=TSS_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==TSS_CURRENT) || (q==TSS_NEW) || (q==TSS_ASK)) sectionstrategy=q;
		}
	}
	// update note in editor strategy
	if (substring_from_line("^[ 	]*updatenoteineditor[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="UNIE_NO") updatenoteineditor=UNIE_NO;
		else if (confdefs[1]=="UNIE_YES") updatenoteineditor=UNIE_YES;
		else if (confdefs[1]=="UNIE_ASK") updatenoteineditor=UNIE_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==UNIE_NO) || (q==UNIE_YES) || (q==UNIE_ASK)) updatenoteineditor=q;
		}
	}
	// AL current work day time worked estimation strategy
	if (substring_from_line("^[ 	]*alcurdayworkedstrategy[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="ACW_TIME") alcurdayworkedstrategy=ACW_TIME;
		else if (confdefs[1]=="ACW_TL") alcurdayworkedstrategy=ACW_TL;
		else if (confdefs[1]=="ACW_ASK") alcurdayworkedstrategy=ACW_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==ACW_TIME) || (q==ACW_TL) || (q==ACW_ASK)) alcurdayworkedstrategy=q;
		}
	}
	// automatically update AL and completion ratios strategy
	if (substring_from_line("^[ 	]*alautoupdate[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="AAU_NO") alautoupdate=AAU_NO;
		else if (confdefs[1]=="AAU_YES") alautoupdate=AAU_YES;
		else if (confdefs[1]=="AAU_ASK") alautoupdate=AAU_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==AAU_NO) || (q==AAU_YES) || (q==AAU_ASK)) alautoupdate=q;
		}
	}
	// automatically update AL and completion ratios if task chunk significantly exceeded strategy
	if (substring_from_line("^[ 	]*alautoupdate_TCexceeded[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="AAU_NO") alautoupdate_TCexceeded=AAU_NO;
		else if (confdefs[1]=="AAU_YES") alautoupdate_TCexceeded=AAU_YES;
		else if (confdefs[1]=="AAU_ASK") alautoupdate_TCexceeded=AAU_ASK;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==AAU_NO) || (q==AAU_YES) || (q==AAU_ASK)) alautoupdate_TCexceeded=q;
		}
	}
	// selected AL Day distribution function
	if (substring_from_line("^[ 	]*aldaydistfunc[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="ALDAYDISTFUNCLINEAR") aldaydistfunc=ALDAYDISTFUNCLINEAR;
		else if (confdefs[1]=="ALDAYDISTFUNCSQUARED") aldaydistfunc=ALDAYDISTFUNCSQUARED;
		else if (confdefs[1].matches(BRXint)) {
			int q = atoi((const char *) confdefs[1]);
			if ((q==ALDAYDISTFUNCLINEAR) || (q==ALDAYDISTFUNCSQUARED)) aldaydistfunc=q;
		}
	}
	// selected AL Cumulative Time Requested HTML form limitation
	if (substring_from_line("^[ 	]*alCRTlimit[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
		if (confdefs[1]=="NONE") alCRTlimit=0;
		else if (confdefs[1]=="ALDAYSDATE") alCRTlimit=1;
		else if (confdefs[1]=="ALHASTD") alCRTlimit=2;
    else if (confdefs[1]=="CRTDAYS") alCRTlimit=3;
	}
	// number of days forward to show in the CTR AL page
	if (substring_from_line("^[ 	]*alCTRdays[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCTRdays=atoi((const char *) confdefs[1]);
  // the timeout to use for separate UI_Info processes of INFO and WARN type
	if (substring_from_line("^[ 	]*ui_info_warn_timeout[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) ui_info_warn_timeout=atoi((const char *) confdefs[1]);
	// multiplier to use to generate CRT AL with safe avaiable task chunk map size
	if (substring_from_line("^[ 	]*alCRTmultiplier[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCRTmultiplier=atoi((const char *) confdefs[1]);
	// offset to add to suggested EPS group variable target dates to preserve grouping
	if (substring_from_line("^[ 	]*alCRTepsgroupoffset[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCRTepsgroupoffset=atoi((const char *) confdefs[1]);
	// update note in editor command line
	if (substring_from_line("^[ 	]*updatenoteineditorcmd[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) updatenoteineditorcmd=confdefs[1];
	// default TL section size
	if (substring_from_line("^[ 	]*sectionsize[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) sectionsize=atoi((const char *) confdefs[1]);
	// default number of AL TCs to generate
	if (substring_from_line("^[ 	]*generatealtcs[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) generatealtcs=atol((const char *) confdefs[1]);
	// default number of AL TCs to show in the shorter listing
	if (substring_from_line("^[ 	]*alfocused[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alfocused=atoi((const char *) confdefs[1]);
	// default character columns for TCs in Calendar_Day at top of AL
	if (substring_from_line("^[ 	]*alfocusedCALexcerptlength[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alfocusedCALexcerptlength=atoi((const char *) confdefs[1]);
	// default number of days for TCs in Calendar_Day at top of AL
	if (substring_from_line("^[ 	]*alfocusedCALdays[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alfocusedCALdays=atoi((const char *) confdefs[1]);
	// default number of columns for TCs in Calendar_Day at top of AL
	if (substring_from_line("^[ 	]*alfocusedCALcolumns[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alfocusedCALcolumns=atoi((const char *) confdefs[1]);
	// default character columns for TCs in focused AL
	if (substring_from_line("^[ 	]*alfocusedexcerptlength[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alfocusedexcerptlength=atoi((const char *) confdefs[1]);
	// default number of AL TCs to show in the longer listing
	if (substring_from_line("^[ 	]*alwide[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alwide=atoi((const char *) confdefs[1]);
	// default number of hours in a regular AL-day
	if (substring_from_line("^[ 	]*algenregular[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) algenregular=atoi((const char *) confdefs[1]);
	// default number of hours in an AL-day for an AL with all DIL-entries
	if (substring_from_line("^[ 	]*algenall[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) algenall=atoi((const char *) confdefs[1]);
	// assumed available hours per day for EPS scheduling method
	if (substring_from_line("^[ 	]*alepshours[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alepshours=atof((const char *) confdefs[1]);
	// default standard start time of work on a regular work day (seconds since midnight)
	if (substring_from_line("^[ 	]*alworkdaystart[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alworkdaystart=atol((const char *) confdefs[1]);
	// default do-later end of day time for prioritypreferences in AL CRT target date proposals
	if (substring_from_line("^[ 	]*dolaterendofday[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) dolaterendofday=atol((const char *) confdefs[1]);
	// default do-earlier end of day time for prioritypreferences in AL CRT target date proposals
	if (substring_from_line("^[ 	]*doearlierendofday[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) doearlierendofday=atol((const char *) confdefs[1]);
	// default allowable desynchronization between AL suggested start and current time in seconds
	if (substring_from_line("^[ 	]*alsyncslack[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alsyncslack=atol((const char *) confdefs[1]);
	// default slope of AL Day distribution
	if (substring_from_line("^[ 	]*aldaydistslope[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) aldaydistslope=atof((const char *) confdefs[1]);
	// limitation of number of days for which periodic tasks are included by repetition during AL generation
	if (substring_from_line("^[ 	]*alperiodictasksincludedayslimit[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alperiodictasksincludedayslimit=atoi((const char *) confdefs[1]);
	// command used to set up an at-job
	if (substring_from_line("^[ 	]*atcommand[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) atcommand=confdefs[1];
  // command used to show a task history
  if (substring_from_line("^[ 	]*taskhistorycall[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) taskhistorycall=confdefs[1];
	// default depth to follow links in a search
	if (substring_from_line("^[ 	]*followlinks[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) followlinks=atoi((const char *) confdefs[1]);
	// maximum width of GraphViz output
	if (substring_from_line("^[ 	]*maxdotgraphwidth[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) maxdotgraphwidth=atoi((const char *) confdefs[1]);
	// HTML form interface
	if (substring_from_line("^[ 	]*htmlforminterface[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) htmlforminterface=confdefs[1];
	// HTML form method
	if (substring_from_line("^[ 	]*htmlformmethod[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) htmlformmethod=confdefs[1];
	// default length of text excerpt from DIL entry in row of Comulative Time Required form
	if (substring_from_line("^[ 	]*alcumtimereqexcerptlength[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alcumtimereqexcerptlength=atoi((const char *) confdefs[1]);
	// default depth of DIL hierarchy
	if (substring_from_line("^[ 	]*hierarchymaxdepth[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) hierarchymaxdepth=atoi((const char *) confdefs[1]);
	// default length of text excerpt from DIL entry in row of DIL hierarchy
	if (substring_from_line("^[ 	]*hierarchyexcerptlength[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) hierarchyexcerptlength=atoi((const char *) confdefs[1]);
	// only include DIL entries with a specific @label:LABEL@ in a hierarchy representation
	if (substring_from_line("^[ 	]*hierarchylabel[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) {
	  hierarchylabel=confdefs[1];
	  if (!hierarchylabel.empty()) hierarchylabel = ("@label:"+hierarchylabel)+'@';
	}
	// alternative mainmetricscategoriesfile
	if (substring_from_line("^[ 	]*mainmetricscategoriesfile[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) mainmetricscategoriesfile=confdefs[1];
	// alternative metricsdayspage
	if (substring_from_line("^[ 	]*metricsdayspage[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) metricsdayspage=confdefs[1];
	// alternative systemselfevaluationdata file
	if (substring_from_line("^[ 	]*systemselfevaluationdata[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) systemselfevaluationdata=confdefs[1];
	// alternative metrics categories RC reproduction file
	if (substring_from_line("^[ 	]*metricsrcreproduction[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) metricsrcreproduction=confdefs[1];
	// earliest day in the Task Log to collect metrics from for -zMAP
	if (substring_from_line("^[ 	]*metricsearliestday[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) metricsearliestday=time_stamp_time(confdefs[1]);
	// do not use X Windows (even if it is available)
	if (pattern_in_line("^[ 	]*noX[ 	]*",lbuf)) noX=true;
	// guarantee that stdin & stdout are used when called from FOR
	if (pattern_in_line("^[ 	]*formguaranteestdinout[ 	]*",lbuf)) formguaranteestdinout=true;
	if (pattern_in_line("^[ 	]*noformguaranteestdinout[ 	]*",lbuf)) formguaranteestdinout=false;
	// Prepend UI_Info derived output with a temporal order count
	if (pattern_in_line("^[ 	]*showuicounter[ 	]*",lbuf)) showuicounter=true;
	if (pattern_in_line("^[ 	]*noshowuicounter[ 	]*",lbuf)) showuicounter=false;	
	// simplify user interaction by reducing requests and options
	if (pattern_in_line("^[ 	]*simplifyuserrequests[ 	]*",lbuf)) simplifyuserrequests=true;
	if (pattern_in_line("^[ 	]*nosimplifyuserrequests[ 	]*",lbuf)) simplifyuserrequests=false;	
	// be strict about time stamps (probably wise)
	if (pattern_in_line("^[ 	]*stricttimestamps[ 	]*",lbuf)) stricttimestamps=true;
	if (pattern_in_line("^[ 	]*nostricttimestamps[ 	]*",lbuf)) stricttimestamps=false;
	// automatically expand number of hours per day as necessary
	if (pattern_in_line("^[ 	]*alautoexpand[ 	]*",lbuf)) alautoexpand=true;
	if (pattern_in_line("^[ 	]*noalautoexpand[ 	]*",lbuf)) alautoexpand=false;
	if (pattern_in_line("^[ 	]*tltrackperformance[ 	]*",lbuf)) tltrackperformance=true;
	if (pattern_in_line("^[ 	]*notltrackperformance[ 	]*",lbuf)) tltrackperformance=false;
	// scheduled controller call at-job command line
	if (substring_from_line("^[ 	]*atcontroller[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) atcontroller=confdefs[1];
	if (pattern_in_line("^[ 	]*suggestnameref[ 	]*",lbuf)) suggestnameref=true;
	if (pattern_in_line("^[ 	]*askprocesstype[ 	]*",lbuf)) askprocesstype=true;
	if (pattern_in_line("^[ 	]*offercreate[ 	]*",lbuf)) offercreate=true;
	if (pattern_in_line("^[ 	]*askALDILref[ 	]*",lbuf)) askALDILref=true;
	if (pattern_in_line("^[ 	]*showflag[ 	]*",lbuf)) showflag=true;
	if (substring_from_line("^[ 	]*nexttaskbrowserpage[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) nexttaskbrowserpage=confdefs[1];
	if (substring_from_line("^[ 	]*flagcmd[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) flagcmd=confdefs[1];
	if (pattern_in_line("^[ 	]*noshowflag[ 	]*",lbuf)) showflag=false;
	if (pattern_in_line("^[ 	]*isdaemon[ 	]*",lbuf)) isdaemon=true;
	if (pattern_in_line("^[ 	]*useansi[ 	]*",lbuf)) useansi=true;
	if (pattern_in_line("^[ 	]*nouseansi[ 	]*",lbuf)) useansi=false;
	if (pattern_in_line("^[ 	]*askextractconcat[ 	]*",lbuf)) askextractconcat=true;
	if (pattern_in_line("^[ 	]*noaskextractconcat[ 	]*",lbuf)) askextractconcat=false;
	if (pattern_in_line("^[ 	]*searchprogressmeter[ 	]*",lbuf)) searchprogressmeter=true;
	if (pattern_in_line("^[ 	]*nosearchprogressmeter[ 	]*",lbuf)) searchprogressmeter=false;
	if (pattern_in_line("^[ 	]*immediatesearchoutput[ 	]*",lbuf)) immediatesearchoutput=true;
	if (pattern_in_line("^[ 	]*noimmediatesearchoutput[ 	]*",lbuf)) immediatesearchoutput=false;
	if (pattern_in_line("^[ 	]*remotesearch[ 	]*",lbuf)) remotesearch=true;
	if (pattern_in_line("^[ 	]*noremotesearch[ 	]*",lbuf)) remotesearch=false;
	if (pattern_in_line("^[ 	]*alshowcumulativereq[ 	]*",lbuf)) alshowcumulativereq=true;
	if (pattern_in_line("^[ 	]*noalshowcumulativereq[ 	]*",lbuf)) alshowcumulativereq=false;
	if (pattern_in_line("^[ 	]*rapidscheduleupdating[ 	]*",lbuf)) rapidscheduleupdating=true;
	if (pattern_in_line("^[ 	]*norapidscheduleupdating[ 	]*",lbuf)) rapidscheduleupdating=false;
	if (pattern_in_line("^[ 	]*targetdatepreferences[ 	]*",lbuf)) targetdatepreferences=true;
	if (pattern_in_line("^[ 	]*notargetdatepreferences[ 	]*",lbuf)) targetdatepreferences=false;
	if (pattern_in_line("^[ 	]*noprioritypreferences[ 	]*",lbuf)) prioritypreferences=false;
	if (pattern_in_line("^[ 	]*prioritypreferences[ 	]*",lbuf)) prioritypreferences=true;
	if (pattern_in_line("^[ 	]*metrics_are_ratios[ 	]*",lbuf)) metrics_are_ratios=true;
	if (pattern_in_line("^[ 	]*nometrics_are_ratios[ 	]*",lbuf)) metrics_are_ratios=false;
	if (pattern_in_line("^[ 	]*labeled_metric_strict_invalidate[ 	]*",lbuf)) labeled_metric_strict_invalidate=true;
	if (pattern_in_line("^[ 	]*nolabeled_metric_strict_invalidate[ 	]*",lbuf)) labeled_metric_strict_invalidate=false;
	// the following seven parameters detected set preferred times for each day of the week at which the work day ends (used in the RSU method)
	if (substring_from_line("^[ 	]*sundayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[0] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*mondayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[1] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*tuesdayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[2] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*wednesdayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[3] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*thursdayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[4] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*fridayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[5] = work_end_preferences(confdefs[1]);
	if (substring_from_line("^[ 	]*saturdayendwork[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) wdworkendprefs[6] = work_end_preferences(confdefs[1]);
	if (pattern_in_line("^[ 	]*usequickloadcache[ 	]*",lbuf)) usequickloadcache=true;
	if (pattern_in_line("^[ 	]*nousequickloadcache[ 	]*",lbuf)) usequickloadcache=false;
	if (pattern_in_line("^[ 	]*immediatecachewriteback[ 	]*",lbuf)) immediatecachewriteback=true;
	if (pattern_in_line("^[ 	]*noimmediatecachewriteback[ 	]*",lbuf)) immediatecachewriteback=false;
	if (pattern_in_line("^[ 	]*reversehierarchy[ 	]*",lbuf)) reversehierarchy=true;
	if (pattern_in_line("^[ 	]*hierarchyaddnonobjectives[ 	]*",lbuf)) hierarchyaddnonobjectives=true;
	if (pattern_in_line("^[ 	]*dotgraph[ 	]*",lbuf)) dotgraph=true;
	if (pattern_in_line("^[ 	]*noreversehierarchy[ 	]*",lbuf)) reversehierarchy=false;
	if (pattern_in_line("^[ 	]*nohierarchyaddnonobjectives[ 	]*",lbuf)) hierarchyaddnonobjectives=false;
	if (pattern_in_line("^[ 	]*nodotgraph[ 	]*",lbuf)) dotgraph=false;
	if (pattern_in_line("^[ 	]*hierarchyplanparse[ 	]*",lbuf)) hierarchyplanparse=true;
	if (pattern_in_line("^[ 	]*nohierarchyplanparse[ 	]*",lbuf)) hierarchyplanparse=false;
	if (pattern_in_line("^[ 	]*hierarchysorting[ 	]*",lbuf)) hierarchysorting=true;
	if (pattern_in_line("^[ 	]*nohierarchysorting[ 	]*",lbuf)) hierarchysorting=false;
	if (pattern_in_line("^[ 	]*periodicautoupdate[ 	]*",lbuf)) periodicautoupdate=true;
	if (pattern_in_line("^[ 	]*periodicadvancebyskip[ 	]*",lbuf)) periodicadvancebyskip=true;
	if (pattern_in_line("^[ 	]*noperiodicautoupdate[ 	]*",lbuf)) periodicautoupdate=false;
	if (pattern_in_line("^[ 	]*alCTRGoogleCalendar[ 	]*",lbuf)) alCTRGoogleCalendar=true;
	if (pattern_in_line("^[ 	]*noalCTRGoogleCalendar[ 	]*",lbuf)) alCTRGoogleCalendar=false;
  if (pattern_in_line("^[ 	]*ctrshowdaycumulative[ 	]*",lbuf)) ctrshowdaycumulative=true;
	if (pattern_in_line("^[ 	]*ctrshowall[ 	]*",lbuf)) ctrshowall=true;
	if (pattern_in_line("^[ 	]*noctrshowall[ 	]*",lbuf)) ctrshowall=false;
	if (substring_from_line("^[ 	]*alCTRGoogleCalendarvaluationcriterion[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCTRGoogleCalendarvaluationcriterion=atof((const char *) confdefs[1]);
	if (substring_from_line("^[ 	]*alCTRGoogleCalendartimecriterion[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCTRGoogleCalendartimecriterion=atof((const char *) confdefs[1]);
	if (substring_from_line("^[ 	]*alCTRGoogleCalendardaysextent[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCTRGoogleCalendardaysextent=atoi((const char *) confdefs[1]);
	if (substring_from_line("^[ 	]*alCTRGCtimezoneadjust[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) alCTRGCtimezoneadjust=atoi((const char *) confdefs[1]);
	if (substring_from_line("^[ 	]*ImmediateTD[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) ImmediateTD=time_stamp_time(confdefs[1]);
	if (substring_from_line("^[ 	]*UrgentTD[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) UrgentTD=time_stamp_time(confdefs[1]);
	if (substring_from_line("^[ 	]*NearTermTD[ 	]*=[ 	]*(.+)$",2,confdefs,lbuf)) NearTermTD=time_stamp_time(confdefs[1]);
}

void initialize() {
  const int LLEN = 10240;
  char lbuf[LLEN];
  numDILs = 0;
  projid = ((time(NULL)/256)+time(NULL)) | 0xFF;
  //info = new UI_Info_Classic(); // This is now set in the variable initialization above.
  bibindexfilename = bibindexfile.after("/",-1);
  if (bibindexfilename=="") bibindexfilename = bibindexfile;
  //	cout << getenv("HOME") << endl;
  // if getenv("HOME")==NULL then use value specified at compile-time
  if (getenv("HOME")!=NULL) homedir = getenv("HOME");
  homedir += '/';
  basedir.prepend(homedir);
  listfile.prepend(basedir);
  idfile.prepend(basedir);
  showmetricsflagfile.prepend(homedir);
  taskhistorypage.prepend(basedir);
  metricsdayspage.prepend(basedir);
  mainmetricscategoriesfile.prepend(basedir);
  metricsmapfile.prepend(homedir);
  systemselfevaluationdata.prepend(homedir);
  metricsrcreproduction.prepend(homedir);
  notetmpfile.prepend(homedir);
  tlentrytmpfile.prepend(homedir);
  configfile.prepend(homedir);
  tltrackperformancefile.prepend(homedir);
  dilpresetfile.prepend(homedir);
  normalskipexcludesfile.prepend(homedir);
  virtualoverlapsfile.prepend(basedir);
  exactconflictsfile.prepend(basedir);
  tasklog.prepend(basedir);
  firstcallpagetemplate.prepend(basedir);
  firstcallpagetarget.prepend(basedir);
  paperplansfile.prepend(basedir);
  poolfile.prepend(basedir);
  ooplannerfile.prepend(basedir);
  figuresdirectory.prepend(basedir);
  bibindexfile.prepend(basedir);
  articleheadtemplate.prepend(basedir);
  articletailtemplate.prepend(basedir);
  templatearticlesubsection.prepend(basedir);
  templatearticlesubsubsection.prepend(basedir);
  dilref.prepend(homedir);
  tareadfile.prepend(homedir);
  finnotesfile.prepend(basedir);
  calendarfile.prepend(basedir);
  // parse configuration file
  ifstream cf(configfile);
  while (cf) {
    cf.getline(lbuf,LLEN);
    if (cf.eof()) break;
    Set_Parameter(lbuf);
  }
  // set dependent variables
  lockidfile = idfile + ".lock";
  crcidfile = idfile + ".crc";
  cacheidfile = idfile + ".cache";
  //	cout << listfile << '\n';
}

bool Diagnostic(String diagnosticstr) {
  // run a self-diagnostic test
  if (diagnosticstr=="cache") {
    Detailed_Items_List dilist;
    return dilist.Binary_Cache_Diagnostic();
  }
  if (diagnosticstr=="test") {
    return test_code();
  }
  return false;
}


String generate_tag(String & tagid) {
	const int NUMTAGS = 8, TAGIDSSIZE= 6, TAGSTRSIZE = 300;
	const char tagids[NUMTAGS][TAGIDSSIZE] = {"CR","GUIDE","LSTY","CHL","CHM","OUTL","EMBH","DESUP"};
	const char tagstr[NUMTAGS][TAGSTRSIZE] = {"<A NAME=\"TAGname\"><!-- @CRef@ -->TAGanchor</A>",
										"<A NAME=\"TAGname\"><!-- @guideline@ -->TAGanchor</A>",
										"% @LaTeX STYLE REQUIRED: <A HREF=\"relstylepath\">style.sty</A>@",
										"% @Begin CHECKLIST: LISTcaption@\n\\checklist{LISTcaption}{\n% --><TABLE BORDER=\"1\"><CAPTION><B>LISTcaption</B></CAPTION><!--\n}\n% @End CHECKLIST: LISTcaption@ --></TABLE><!--",
										"% @Begin CHECKMARK: MARKtype MARKanchor@\n\\checkmark{0}{MARKtext} (MARKanchor)\\\\\n% --><TR><TD><INPUT TYPE=checkbox NAME=\"MARKtype\" VALUE=\"MARKanchor\"> MARKtext (<A HREF=\"relmarkpath\">MARKanchor</A>)</TD></TR><!--\n% @End CHECKMARK: MARKtype MARKanchor@",
										"% @Begin OUTLINE: Context@\n\\outlinenote{OutlineData}\n% @End OUTLINE: Context@",
										"% @EMBEDDED HTML: HTMLtext@",
										"@DIL Entry Superior: <A HREF=\"#SUPid\">SUPid</A> (SIrel,SIuimp,SIimp,SItd,SIurg,SIpri)@"};
	if (tagid=="") {
		String res;
		for (int i = 0; i<NUMTAGS; i++) res += String(tagids[i])+" => "+String(tagstr[i])+'\n';
		return res;
	}
	for (int i=0; i<NUMTAGS; i++) if (tagid==tagids[i]) return String(tagstr[i]);
	return String("");
}

void exit_report() {
	VERYVERBOSEOUT("Resolving exit hook: exit_report\n");
    // loop warnings
    if (looplist) {
		COMPILEDPING(std::cout,"PING-exit_report-looplist\n");
        ADDWARNING(__func__, "DIL_entry::Propagated_Target_Date(), loops detected at:");
        PLL_LOOP_FORWARD(DIL_entry, looplist->list.head(), 1) {
			//std::string loopstr = e->chars();
			//ADDWARNING("DIL#",loopstr);
			ADDWARNING(__func__,"Cannot report DIL# here - this caused segfaults.");
        }
		COMPILEDPING(std::cout,"PING-exit_report-looplist-4\n");
        delete looplist;
		looplist = nullptr;
    }
	COMPILEDPING(std::cout,"PING-exit_report-done\n");
}

#ifdef INCLUDE_PROFILING_TIMERS
#include <sys/time.h>
void resettimer() {
	struct itimerval itval;
	itval.it_interval.tv_sec = 3600;
	itval.it_interval.tv_usec = 500000;
	itval.it_value.tv_sec = 3600;
	itval.it_value.tv_usec = 500000;
	setitimer(ITIMER_PROF,&itval,NULL);
}
void readtimer(long & sec, long & usec) {
	struct itimerval itout;
	getitimer(ITIMER_PROF,&itout);
	usec = 500000 - itout.it_value.tv_usec;
	if (usec<0) {
		usec += 1000000;
		itout.it_value.tv_sec++;
	}
	sec = 3600 - itout.it_value.tv_sec;
}
#endif

void exit_postop() {
  VERYVERBOSEOUT("Resolving exit hook: exit_postop\n");
  // This does all of the things that we really should do for a clean
  // exit. For example, it flushes user interfaces, which insures that
  // all messages are received.
  // It should probably also ensure that everything is in a usable
  // state upon closing the program.

  // Flush the HTML log file
  Output_Log_Flush(); // Do this first, so that errors still go to ERROR

  // Flushing UI_ objects
  if (info) INFO.flush();
  if (error) ERROR.flush();
  if (warn) WARN.flush();

  // Flushing IO streams (even though they probably already do that upon exit)
  if (eout) EOUT.flush();
  if (vout) VOUT.flush();
}
