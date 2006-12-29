/* This file is part of the KDE project
   Copyright (C) 1998-2003 The KSpread Team <koffice-devel@kde.org>
   Copyright (C) 2005 Tomas Mecir <mecirt@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

// built-in date/time functions

#include "Doc.h"
#include "Functions.h"
#include "functions/helper.h"
#include "ValueCalc.h"
#include "ValueConverter.h"


#include <kcalendarsystem.h>
#include <klocale.h>

using namespace KSpread;

// prototypes, sorted
Value func_currentDate (valVector args, ValueCalc *calc, FuncExtra *);
Value func_currentDateTime (valVector args, ValueCalc *calc, FuncExtra *);
Value func_currentTime (valVector args, ValueCalc *calc, FuncExtra *);
Value func_date (valVector args, ValueCalc *calc, FuncExtra *);
Value func_date2unix (valVector args, ValueCalc *calc, FuncExtra *);
Value func_dateDif (valVector args, ValueCalc *calc, FuncExtra *);
Value func_datevalue (valVector args, ValueCalc *calc, FuncExtra *);
Value func_day (valVector args, ValueCalc *calc, FuncExtra *);
Value func_dayname (valVector args, ValueCalc *calc, FuncExtra *);
Value func_dayOfYear (valVector args, ValueCalc *calc, FuncExtra *);
Value func_days (valVector args, ValueCalc *calc, FuncExtra *);
Value func_days360 (valVector args, ValueCalc *calc, FuncExtra *);
Value func_daysInMonth (valVector args, ValueCalc *calc, FuncExtra *);
Value func_daysInYear  (valVector args, ValueCalc *calc, FuncExtra *);
Value func_easterSunday (valVector args, ValueCalc *calc, FuncExtra *);
Value func_edate (valVector args, ValueCalc *calc, FuncExtra *);
Value func_eomonth (valVector args, ValueCalc *calc, FuncExtra *);
Value func_hour (valVector args, ValueCalc *calc, FuncExtra *);
Value func_hours (valVector args, ValueCalc *calc, FuncExtra *);
Value func_isLeapYear  (valVector args, ValueCalc *calc, FuncExtra *);
Value func_isoWeekNum (valVector args, ValueCalc *calc, FuncExtra *);
Value func_minute (valVector args, ValueCalc *calc, FuncExtra *);
Value func_minutes (valVector args, ValueCalc *calc, FuncExtra *);
Value func_month (valVector args, ValueCalc *calc, FuncExtra *);
Value func_monthname (valVector args, ValueCalc *calc, FuncExtra *);
Value func_months (valVector args, ValueCalc *calc, FuncExtra *);
Value func_networkday (valVector args, ValueCalc *calc, FuncExtra *);
Value func_second (valVector args, ValueCalc *calc, FuncExtra *);
Value func_seconds (valVector args, ValueCalc *calc, FuncExtra *);
Value func_time (valVector args, ValueCalc *calc, FuncExtra *);
Value func_timevalue (valVector args, ValueCalc *calc, FuncExtra *);
Value func_today (valVector args, ValueCalc *calc, FuncExtra *);
Value func_unix2date (valVector args, ValueCalc *calc, FuncExtra *);
Value func_weekday (valVector args, ValueCalc *calc, FuncExtra *);
Value func_weekNum (valVector args, ValueCalc *calc, FuncExtra *);
Value func_weeks (valVector args, ValueCalc *calc, FuncExtra *);
Value func_weeksInYear (valVector args, ValueCalc *calc, FuncExtra *);
Value func_workday (valVector args, ValueCalc *calc, FuncExtra *);
Value func_year (valVector args, ValueCalc *calc, FuncExtra *);
Value func_yearFrac (valVector args, ValueCalc *calc, FuncExtra *);
Value func_years (valVector args, ValueCalc *calc, FuncExtra *);

// registers all date/time functions
// sadly, many of these functions aren't Excel compatible
void RegisterDateTimeFunctions()
{
  FunctionRepository* repo = FunctionRepository::self();
  Function *f;

  f = new Function ("CURRENTDATE",  func_currentDate);
  f->setParamCount (0);
  repo->add (f);
  f = new Function ("CURRENTDATETIME",  func_currentDateTime);
  f->setParamCount (0);
  repo->add (f);
  f = new Function ("CURRENTTIME",  func_currentTime);
  f->setParamCount (0);
  repo->add (f);
  f = new Function ("DATE",  func_date);
  f->setParamCount (3);
  f = new Function ("DATE2UNIX",  func_date2unix);
  f->setParamCount (1);
  repo->add (f);
  f = new Function ("DATEDIF",  func_dateDif);
  f->setParamCount (3);
  repo->add (f);
  f = new Function ("DATEVALUE",  func_datevalue);
  repo->add (f);
  f = new Function ("DAY",  func_day);
  repo->add (f);
  f = new Function ("DAYNAME",  func_dayname);
  repo->add (f);
  f = new Function ("DAYOFYEAR",  func_dayOfYear);
  f->setParamCount (3);
  repo->add (f);
  f = new Function ("DAYS",  func_days);
  f->setParamCount (2);
  repo->add (f);
  f = new Function ("DAYS360",  func_days360);
  f->setParamCount (2, 3);
  repo->add (f);
  f = new Function ("DAYSINMONTH",  func_daysInMonth);
  f->setParamCount (2);
  repo->add (f);
  f = new Function ("DAYSINYEAR",  func_daysInYear);
  repo->add (f);
  f = new Function ("EASTERSUNDAY",  func_easterSunday);
  repo->add (f);
  f = new Function ("EDATE",  func_edate);
  f->setParamCount (2);
  repo->add (f);
  f = new Function ("EOMONTH",  func_eomonth);
  f->setParamCount (2);
  repo->add (f);
  f = new Function ("HOUR",  func_hour);
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("HOURS",  func_hour);  // same as HOUR
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("ISLEAPYEAR",  func_isLeapYear);
  repo->add (f);
  f = new Function ("ISOWEEKNUM",  func_isoWeekNum);
  repo->add (f);
  f = new Function ("MINUTE",  func_minute);
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("MINUTES",  func_minute);  // same as MINUTE
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("MONTH",  func_month);
  repo->add (f);
  f = new Function ("MONTHNAME",  func_monthname);
  repo->add (f);
  f = new Function ("MONTHS",  func_months);
  f->setParamCount (3);
  repo->add (f);
  f = new Function ("NETWORKDAY",  func_networkday);
  f->setParamCount (2, 3);
  f->setAcceptArray();
  repo->add (f);
  f = new Function ("NOW",  func_currentDateTime);
  f->setParamCount (0);
  repo->add (f);
  f = new Function ("SECOND",  func_second);
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("SECONDS",  func_second);  // same as SECOND
  f->setParamCount (0, 1);
  repo->add (f);
  f = new Function ("TIME",  func_time);
  f->setParamCount (3);
  repo->add (f);
  f = new Function ("TIMEVALUE",  func_timevalue);
  repo->add (f);
  f = new Function ("TODAY",  func_currentDate);
  f->setParamCount (0);
  repo->add (f);
  f = new Function ("UNIX2DATE",  func_unix2date);
  f->setParamCount (1);
  repo->add (f);
  f = new Function ("WEEKDAY",  func_weekday);
  f->setParamCount (1, 2);
  repo->add (f);
  f = new Function ("WEEKNUM",  func_weekNum);
  f->setParamCount (1, 2);
  repo->add (f);
  f = new Function ("WEEKS",  func_weeks);
  f->setParamCount (3);
  repo->add (f);
  f = new Function ("WEEKSINYEAR",  func_weeksInYear);
  repo->add (f);
  f = new Function ("WORKDAY",  func_workday);
  f->setParamCount (2, 3);
  f->setAcceptArray();
  repo->add (f);
  f = new Function ("YEAR",   func_year);
  repo->add (f);
  f = new Function ("YEARFRAC",  func_yearFrac);
  f->setParamCount (2, 3);
  repo->add (f);
  f = new Function ("YEARS",  func_years);
  f->setParamCount (3);
  repo->add (f);
}

// Function: EDATE
Value func_edate (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  int months = calc->conv()->asInteger (args[1]).asInteger();

  date = calc->conv()->locale()->calendar()->addMonths (date, months);

  if (!date.isValid())
    return Value::errorVALUE();

  return Value( date, calc->doc() );
}

// Function: EOMONTH
Value func_eomonth (valVector args, ValueCalc *calc, FuncExtra *)
{
  // add months to date using EDATE
  Value modDate = func_edate (args, calc, 0);
  if (modDate.isError()) return modDate;

  // modDate is currently in Date format
  QDate date = modDate.asDate( calc->doc() );
  date.setYMD (date.year(), date.month(), date.daysInMonth());

  return Value( date, calc->doc() );
}

// internal helper function
static int func_days360_helper( const QDate& _date1, const QDate& _date2, bool european )
{
  int day1, day2;
  int month1, month2;
  int year1, year2;
  bool negative = false;
  QDate date1( _date1 );
  QDate date2( _date2 );

  if (date1.daysTo( date2 ) < 0)
  {
    QDate tmp( date1 );
    date1 = date2;
    date2 = tmp;
    negative = true;
  }

  day1   = date1.day();
  day2   = date2.day();
  month1 = date1.month();
  month2 = date2.month();
  year1  = date1.year();
  year2  = date2.year();

  if ( european )
  {
    if ( day1 == 31 )
      day1 = 30;
    if ( day2 == 31 )
      day2 = 30;
  }
  else
  {
    // thanks to the Gnumeric developers for this...
    if ( month1 == 2 && month2 == 2
         && date1.daysInMonth() == day1
         && date2.daysInMonth() == day2 )
      day2 = 30;

    if ( month1 == 2 && date1.daysInMonth() == day1 )
      day1 = 30;

    if ( day2 == 31 && day1 >= 30 )
      day2 = 30;

    if ( day1 == 31 )
      day1 = 30;
  }

  return ( ( year2 - year1 ) * 12 + ( month2 - month1 ) ) * 30
    + ( day2 - day1 );
}

// Function: DAYS360
// algorithm adapted from gnumeric
Value func_days360 (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date1 = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  QDate date2 = calc->conv()->asDate (args[1]).asDate( calc->doc() );
  bool european = false;
  if (args.count() == 3)
    european = calc->conv()->asBoolean (args[2]).asBoolean();

  return Value( func_days360_helper( date1, date2, european ) );
}

// Function: YEAR
Value func_year (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v = calc->conv()->asDate (args[0]);
  if (v.isError()) return v;
  QDate date = v.asDate( calc->doc() );
  return Value (date.year ());
}

// Function: MONTH
Value func_month (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v = calc->conv()->asDate (args[0]);
  if (v.isError()) return v;
  QDate date = v.asDate( calc->doc() );
  return Value (date.month ());
}

// Function: DAY
Value func_day (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v = calc->conv()->asDate (args[0]);
  if (v.isError()) return v;
  QDate date = v.asDate( calc->doc() );
  return Value (date.day ());
}

// Function: HOUR
Value func_hour (valVector args, ValueCalc *calc, FuncExtra *)
{
  QTime time;
  if (args.count() == 1)
  {
    Value v = calc->conv()->asTime (args[0]);
    if (v.isError()) return v;
    time = v.asTime( calc->doc() );
  }
  else
    time = QTime::currentTime ();
  return Value (time.hour ());
}

// Function: MINUTE
Value func_minute (valVector args, ValueCalc *calc, FuncExtra *)
{
  QTime time;
  if (args.count() == 1)
  {
    Value v = calc->conv()->asTime (args[0]);
    if (v.isError()) return v;
    time = v.asTime( calc->doc() );
  }
  else
    time = QTime::currentTime ();
  return Value (time.minute ());
}

// Function: SECOND
Value func_second (valVector args, ValueCalc *calc, FuncExtra *)
{
  QTime time;
  if (args.count() == 1)
  {
    Value v = calc->conv()->asTime (args[0]);
    if (v.isError()) return v;
    time = v.asTime( calc->doc() );
  }
  else
    time = QTime::currentTime ();
  return Value (time.second ());
}

// Function: WEEKDAY
Value func_weekday (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v( calc->conv()->asDate (args[0]) );
  if (v.isError()) return v;
  QDate date = v.asDate( calc->doc() );
  int method = 1;
  if (args.count() == 2)
    method = calc->conv()->asInteger (args[1]).asInteger();

  if ( method < 1 || method > 3 )
    return Value::errorVALUE();

  int result = date.dayOfWeek();

  if (method == 3)
    --result;
  else if (method == 1)
  {
    ++result;
    result = result % 7;
  }

  return Value (result);
}

// Function: DATEVALUE
// same result would be obtained by applying number format on a date value
Value func_datevalue (valVector args, ValueCalc *calc, FuncExtra *)
{
  if (args[0].isString()) {
    Value v = calc->conv()->asDate (args[0]);
    if (! v.isError())
      return calc->conv()->asFloat (v);
  }
  return Value::errorVALUE();
}

// Function: timevalue
// same result would be obtained by applying number format on a time value
Value func_timevalue (valVector args, ValueCalc *calc, FuncExtra *)
{
  if (args[0].isString()) {
    Value v = calc->conv()->asTime (args[0]);
    if (! v.isError())
      return calc->conv()->asFloat (v);
  }
  return Value::errorVALUE();
}

// Function: YEARS
Value func_years (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date1 = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  QDate date2 = calc->conv()->asDate (args[1]).asDate( calc->doc() );
  if (!date1.isValid() || !date2.isValid())
    return Value::errorVALUE();

  int type = calc->conv()->asInteger (args[2]).asInteger();
  if (type == 0)
  {
    // max. possible years between both dates
    int years = date2.year() - date1.year();

    if (date2.month() < date1.month())
      --years;
    else if ( (date2.month() == date1.month()) && (date2.day() < date1.day()) )
      --years;

    return Value (years);
  }

  // type is non-zero now
  // the number of full years in between, starting on 1/1/XXXX
  if ( date1.year() == date2.year() )
    return Value (0);

  if ( (date1.month() != 1) || (date1.day() != 1) )
    date1.setYMD(date1.year() + 1, 1, 1);
  date2.setYMD(date2.year(), 1, 1);

  return Value (date2.year() - date1.year());
}

// Function: MONTHS
Value func_months (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date1 = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  QDate date2 = calc->conv()->asDate (args[1]).asDate( calc->doc() );
  if (!date1.isValid() || !date2.isValid())
    return Value::errorVALUE();

  int type = calc->conv()->asInteger (args[2]).asInteger();
  if (type == 0)
  {
    int months  = (date2.year() - date1.year()) * 12;
    months += date2.month() - date1.month();

    if (date2.day() < date1.day())
      if (date2.day() != date2.daysInMonth())
        --months;

    return Value (months);
  }

  // type is now non-zero
  // the number of full months in between, starting on 1/XX/XXXX
  if (date1.month() == 12)
    date1.setYMD(date1.year() + 1, 1, 1);
  else
    date1.setYMD(date1.year(), date1.month() + 1, 1);
  date2.setYMD(date2.year(), date2.month(), 1);

  int months = (date2.year() - date1.year()) * 12;
  months += date2.month() - date1.month();

  return Value (months);
}

// Function: WEEKS
Value func_weeks (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date1 = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  QDate date2 = calc->conv()->asDate (args[1]).asDate( calc->doc() );
  if (!date1.isValid() || !date2.isValid())
    return Value::errorVALUE();

  int type = calc->conv()->asInteger (args[2]).asInteger();
  int days = date1.daysTo (date2);
  if (type == 0)
    // just the number of full weeks between
    return Value ((int) (days / 7));

  // the number of full weeks between starting on mondays
  int weekStartDay = calc->conv()->locale()->weekStartDay();

  int dow1 = date1.dayOfWeek();
  int dow2 = date2.dayOfWeek();

  days -= (7 + (weekStartDay % 7) - dow1);
  days -= ((dow2 - weekStartDay) % 7);

  return Value ((int) (days / 7));
}

// Function: DAYS
Value func_days (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date1 = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  QDate date2 = calc->conv()->asDate (args[1]).asDate( calc->doc() );
  if (!date1.isValid() || !date2.isValid())
    return Value::errorVALUE();

  return Value (date1.daysTo (date2));
}

// Function: DATE
Value func_date (valVector args, ValueCalc *calc, FuncExtra *)
{
  int y = calc->conv()->asInteger (args[0]).asInteger();
  int m = calc->conv()->asInteger (args[1]).asInteger();
  int d = calc->conv()->asInteger (args[2]).asInteger();

  QDate _date;
  if( _date.setYMD (y, m, d))
    return Value( _date, calc->doc() );
  return Value::errorVALUE();
}

// Function: DAY
Value func_dayname (valVector args, ValueCalc *calc, FuncExtra *)
{
  int number = calc->conv()->asInteger (args[0]).asInteger();

  QString weekName = calc->conv()->locale()->calendar()->weekDayName (number);
  if (weekName.isNull())
    return Value::errorVALUE();
  return Value (weekName);
}

// Function: MONTHNAME
Value func_monthname (valVector args, ValueCalc *calc, FuncExtra *)
{
  int number = calc->conv()->asInteger (args[0]).asInteger();

  QString monthName = calc->conv()->locale()->calendar()->monthName (number,
      QDate::currentDate().year());
  if (monthName.isNull())
    return Value::errorVALUE();
  return Value (monthName);
}

// Function: TIME
Value func_time (valVector args, ValueCalc *calc, FuncExtra *)
{
  int h = calc->conv()->asInteger (args[0]).asInteger();
  int m = calc->conv()->asInteger (args[1]).asInteger();
  int s = calc->conv()->asInteger (args[2]).asInteger();

  /* normalize the data */
  m += s / 60;
  s = s % 60;
  h += m / 60;
  m = m % 60;
  // we'll lose hours data that carries over into days
  h = h % 24;

  // now carry down hours/minutes for negative minutes/seconds
  if (s < 0) {
    s += 60;
    m -= 1;
  }
  if (m < 0) {
    m += 60;
    h -= 1;
  }
  if (h < 0)
    h += 24;

  return Value( QTime (h, m, s), calc->doc() );
}

// Function: CURRENTDATE
Value func_currentDate (valVector, ValueCalc * calc, FuncExtra *)
{
  return Value( QDate::currentDate(), calc->doc() );
}

// Function: CURRENTTIME
Value func_currentTime (valVector, ValueCalc * calc, FuncExtra *)
{
  return Value( QTime::currentTime(), calc->doc() );
}

// Function: CURRENTDATETIME
Value func_currentDateTime (valVector, ValueCalc * calc, FuncExtra *)
{
  return Value( QDateTime::currentDateTime(), calc->doc() );
}

// Function: DAYOFYEAR
Value func_dayOfYear (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value date = func_date (args, calc, 0);
  if (date.isError()) return date;
  return Value (date.asDate( calc->doc() ).dayOfYear());
}

// Function: DAYSINMONTH
Value func_daysInMonth (valVector args, ValueCalc *calc, FuncExtra *)
{
  int y = calc->conv()->asInteger (args[0]).asInteger();
  int m = calc->conv()->asInteger (args[1]).asInteger();
  QDate date (y, m, 1);
  return Value (date.daysInMonth());
}

// Function: ISLEAPYEAR
Value func_isLeapYear  (valVector args, ValueCalc *calc, FuncExtra *)
{
  int y = calc->conv()->asInteger (args[0]).asInteger();
  return Value (QDate::isLeapYear (y));
}

// Function: DAYSINYEAR
Value func_daysInYear  (valVector args, ValueCalc *calc, FuncExtra *)
{
  int y = calc->conv()->asInteger (args[0]).asInteger();
  return Value (QDate::isLeapYear (y) ? 366 : 365);
}

// Function: WEEKSINYEAR
Value func_weeksInYear (valVector args, ValueCalc *calc, FuncExtra *)
{
  int y = calc->conv()->asInteger (args[0]).asInteger();
  QDate date (y, 12, 31);  // last day of the year
  return Value (date.weekNumber ());
}

// Function: EASTERSUNDAY
Value func_easterSunday (valVector args, ValueCalc *calc, FuncExtra *)
{
  int nDay, nMonth;
  int nYear = calc->conv()->asInteger (args[0]).asInteger();

  // (Tomas) the person who wrote this should be hanged :>
  int B,C,D,E,F,G,H,I,K,L,M,N,O;
  N = nYear % 19;
  B = int(nYear / 100);
  C = nYear % 100;
  D = int(B / 4);
  E = B % 4;
  F = int((B + 8) / 25);
  G = int((B - F + 1) / 3);
  H = (19 * N + B - D - G + 15) % 30;
  I = int(C / 4);
  K = C % 4;
  L = (32 + 2 * E + 2 * I - H - K) % 7;
  M = int((N + 11 * H + 22 * L) / 451);
  O = H + L - 7 * M + 114;
  nDay = O % 31 + 1;
  nMonth = int(O / 31);

  return Value( QDate (nYear, nMonth, nDay), calc->doc() );
}

// Function: ISOWEEKNUM
Value func_isoWeekNum (valVector args, ValueCalc *calc, FuncExtra *)
{
  QDate date = calc->conv()->asDate (args[0]).asDate( calc->doc() );
  if (!date.isValid())
      return Value::errorVALUE();

  return Value (date.weekNumber());
}

// Function: WEEKNUM
//
// 		method 	startday name of day
// default: 	1	 0	 sunday
//		2	-1	 monday
// 
// weeknum = (startday + 7 + dayOfWeek of New Year + difference in days) / 7
//
Value func_weekNum (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v( calc->conv()->asDate (args[0]) );
  if (v.isError()) return v;
  QDate date = v.asDate( calc->doc() );

  if (!date.isValid())
      return Value::errorVALUE();

  int method = 1;
  if (args.count() > 1)
      method = calc->conv()->asInteger (args[1]).asInteger();

  if ( method < 1 || method > 2 )
      return Value::errorVALUE();

  QDate date1 ( date.year(), 1, 1 );
  int days = date1.daysTo (date);
  
  int startday=0;
  if (method == 2)
    startday=-1;

  //kDebug(36002) << "weeknum = [startday(" << startday << ") + base(7) + New Year(" << date1.dayOfWeek() <<") + days(" << days << ")] / 7 = " << (startday+7+date1.dayOfWeek()+days)/7 << endl; 
  return Value( (int)(startday+7+date1.dayOfWeek()+days)/7 );
}

// Function: DATEDIF
//
// 		interval difference type	descrition 	
// default: 	m	 months
//		d	 days
//		y	 complete years
//		ym	 months excluding years
//		yd	 days excluding years
//		md	 days excluding months and years
//
Value func_dateDif (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v1( calc->conv()->asDate (args[0]) );
  if (v1.isError()) return v1;
  QDate date1 = v1.asDate( calc->doc() );

  if (!date1.isValid())
      return Value::errorVALUE();

  Value v2( calc->conv()->asDate (args[1]) );
  if (v2.isError()) return v2;
  QDate date2 = v2.asDate( calc->doc() );

  if (!date2.isValid())
      return Value::errorVALUE();

  // check if interval is valid
  QString interval = calc->conv()->asString (args[2]).asString();
  if ( !(interval=="m" || interval=="d" || interval=="y" || interval=="ym" || interval=="yd" || interval=="md") )
    return Value::errorVALUE();

  // local vars
  int y,m,d;
  int sign=1; // default
  int res=0;
  
  QDate Temp1, Temp2;
  
  //QDate date0(1899,12,30); // referenceDate
   QDate date0 = calc->doc()->referenceDate();
  if ( date2 < date1 )
  {
    // exchange values and set sign
    Temp1=date1;
    date1=date2;
    date2=Temp1;
    sign=-1;
  }

  //  
  // calculate 
  //
  
  // Temp1 = DateSerial(Year(Date2), Month(Date1), Day(Date1))
  Temp1.setDate(date2.year(), date1.month(), date1.day());

  // Y = Year(Date2) - Year(Date1) + (Temp1 > Date2)
  y = date2.year() - date1.year() + (date0.daysTo(Temp1) > date0.daysTo(date2)?-1:0);
  
  // M = Month(Date2) - Month(Date1) - (12 * (Temp1 > Date2))
  m = date2.month() - date1.month() - (12 * (Temp1 > date2?-1:0));
  
  // D = Day(Date2) - Day(Date1)
  d = date2.day() - date1.day();
  
  if ( d < 0 )
  {
    // M = M - 1
    m--;
    // D = Day(DateSerial(Year(date2), Month(date2), 0)) + D
    Temp2.setDate(date2.year(), date2.month()-1, 1);
    d = Temp2.daysInMonth()+d;
  }
  
  //
  // output
  //
  
  if ( interval == "y" )
  {
    // year
    res = y*sign;
  } 
  else if ( interval == "m" )
  {
    // month
    res = (12*y+m)*sign;
  } 
  else if ( interval == "d" )
  {
    // days
    int days = date0.daysTo(date2)-date0.daysTo(date1);
    res = days*sign;
  } 
  else if ( interval == "ym" )
  {
    // month excl. years
    res = m*sign;
  } 
  else if ( interval == "yd" )
  {
    // days excl. years
    QDate Temp3(date2.year(), date1.month(), date1.day());
    int days = date0.daysTo(date2)-date0.daysTo(Temp3);
    
    res = days*sign;
  } 
  else if ( interval == "md" )
  {
    // days excl. month and years
    res = d*sign;
  }
  return Value( res );
}

// Function: YEARFRAC
//
// 		basis    descritption day-count 	
// default: 	0	 US (NASD) system. 30 days/month, 360 days/year (30/360)
//		1	 Actual/actual (Euro), also known as AFB
//		2	 Actual/360
//		3	 Actual/365
//		4	 European 30/360
//
Value func_yearFrac (valVector args, ValueCalc *calc, FuncExtra *)
{
  Value v1( calc->conv()->asDate (args[0]) );
  if (v1.isError()) return v1;
  QDate date1 = v1.asDate( calc->doc() );

  if (!date1.isValid())
      return Value::errorVALUE();

  Value v2( calc->conv()->asDate (args[1]) );
  if (v2.isError()) return v2;
  QDate date2 = v2.asDate( calc->doc() );

  if (!date2.isValid())
      return Value::errorVALUE();

  // check if basis is valid
  int basis = calc->conv()->asInteger (args[2]).asInteger();
  if ( basis < 0 || basis > 4 ) 
    return Value::errorVALUE();

  //
  // calculation
  //

  QDate date0 = calc->doc()->referenceDate(); // referenceDate

  if ( date2 < date1 )
  {
    // exchange dates
    QDate Temp1=date1;
    date1=date2;
    date2=Temp1;
  }

  int days = date0.daysTo(date2) - date0.daysTo(date1);

  //kDebug(36002) << "date1 = " << date1 << "    date2 = " << date2 << "    days = " << days << "    basis = " << basis << endl;

  double res=0;
  double peryear=0;

  switch(basis)
  {
    case 1:
    {
      // Actual/actual
      int leaps=0,years=0;
      double k;

      if (days < (365 + QDate::isLeapYear(date1.year()) + 1))
      {
        // less than 1 year
        //kDebug(36002) << "less tahn 1 year ..." << endl;

        // bool 1 = 29.2. is in between dates
        k = (QDate::isLeapYear(date1.year()) && date1.month()<3) || (QDate::isLeapYear(date2.year()) && date2.month()*100+date2.day() >= 2*100+29);
        years = 1;
      }
      else
      {
        // more than 1 year
        //kDebug(36002) << "more than 1 year ..." << endl;
        years = date2.year()-date1.year()+1;
        leaps = QDate(date2.year()+1, 1, 1).toJulianDay() - QDate(date1.year(), 1, 1).toJulianDay() - 365*years;
        k = (double)leaps/years;
      }

      //kDebug(36002) << "leaps = " << leaps << "    years = " << years << "    leaps per year = " << (double)leaps/years << endl;
      peryear = 365 + k;
      break;
    }
    case 2:
    {
      // Actual/360
      peryear = 360;
      break;
    }
    case 3:
    {
      // Actual/365
      peryear = 365;
      break;
    }
    case 4:
    {
      // 30/360 Europe

      // calc datedif360 (start, end, Europe)
      days = func_days360_helper( date1, date2, 1);

      peryear = 360;
      break;
    }
    default:
    {
      // NASD 30/360
      basis = 0;

      // calc datedif360 (start, end, US)
      days = func_days360_helper( date1, date2, 0);

      peryear = 360;
    }
  }

  res = (double)days / peryear;

  return Value( res );
}

// Function: WORKDAY 
//
// - negative days count backwards
// - if holidays is not an array it is only added to days (neg. are not allowed)
// 
Value func_workday (valVector args, ValueCalc *calc, FuncExtra *e)
{
  Value v( calc->conv()->asDate (args[0]) );
  
  if (v.isError()) return v;
  QDate startdate = v.asDate( calc->doc() );

  if (!startdate.isValid())
      return Value::errorVALUE();

  //
  // vars 
  //
  int days = calc->conv()->asInteger (args[1]).asInteger();
  
  QDate date0 = calc->doc()->referenceDate(); 	// referenceDate
  QDate enddate = startdate;			// enddate
  valVector holidays; 				// stores holidays
  int sign=1; 					// sign 1 = forward, -1 = backward
  
  if ( days < 0 )
  {
    // change sign and set count to ccw
    days = days * -1;
    sign = -1;
  }
  
  //
  // check for holidays
  //
  if (args.count() > 2)
  {
    if (args[2].type() == Value::Array) 
    { // parameter is array
      unsigned int row1, col1, rows, cols;
     
      row1 = e->ranges[2].row1;
      col1 = e->ranges[2].col1;
      rows = e->ranges[2].row2 - row1 + 1;
      cols = e->ranges[2].col2 - col1 + 1;
        
      Value holiargs = args[2];
	
      for (unsigned r = 0; r < rows; ++r)
	for (unsigned c = 0; c < cols; ++c)
	{
	  // only append if element is a valid date
	  if (!holiargs.element(c+col1, r+row1).isEmpty())
	  {
	    Value v (calc->conv()->asDate (holiargs.element(c+col1, r+row1)));
	    if (v.isError())
	      return Value::errorVALUE();

	    if (v.asDate( calc->doc() ).isValid())
	      holidays.append( v );
	  } 
	}
    } else 
    { // no array parameter
      if (args[2].isString())
      {// isString
        Value v( calc->conv()->asDate (args[2]) );
	if (v.isError()) 
	  return Value::errorVALUE();
	
	if (v.asDate( calc->doc() ).isValid())
	  holidays.append( v );
	
      } else
      {// isNumber 
        int hdays = calc->conv()->asInteger (args[2]).asInteger();

        if ( hdays < 0 )
	  return Value::errorVALUE();
        days = days + hdays;
      }
    }
  }

  //
  // count days
  //
  while(days)
  {
    // exclude weekends and holidays
    do
    {
      enddate = enddate.addDays( 1*sign );
    } while( enddate.dayOfWeek()>5 || holidays.contains(Value (date0.daysTo(enddate)) ) );
    
    days--;
  }

  return Value( enddate.toString() );
}

// Function: NETWORKDAY 
//
// - if holidays is not an array it is only added to days (neg. are not allowed)
// 
Value func_networkday (valVector args, ValueCalc *calc, FuncExtra *e)
{
  Value v1( calc->conv()->asDate (args[0]) );
  
  if (v1.isError()) return v1;
  QDate startdate = v1.asDate( calc->doc() );

  Value v2( calc->conv()->asDate (args[1]) );
  
  if (v2.isError()) return v2;
  QDate enddate = v2.asDate( calc->doc() );

  if (!startdate.isValid() || !enddate.isValid())
      return Value::errorVALUE();

  //
  // vars 
  //
   
  int days = 0;					// workdays
  QDate date0 = calc->doc()->referenceDate(); 	// referenceDate
  valVector holidays; 				// stores holidays
  int sign=1; 					// sign 1 = forward, -1 = backward
  
  if ( enddate < startdate )
  {
    // change sign and set count to ccw
    sign = -1;
  }
    
  //
  // check for holidays
  //
  if (args.count() > 2)
  {
    if (args[2].type() == Value::Array) 
    { // parameter is array
      unsigned int row1, col1, rows, cols;
     
      row1 = e->ranges[2].row1;
      col1 = e->ranges[2].col1;
      rows = e->ranges[2].row2 - row1 + 1;
      cols = e->ranges[2].col2 - col1 + 1;
        
      Value holiargs = args[2];
	
      for (unsigned r = 0; r < rows; ++r)
	for (unsigned c = 0; c < cols; ++c)
	{
	  // only append if element is a valid date
	  if (!holiargs.element(c+col1, r+row1).isEmpty())
	  {
	    Value v (calc->conv()->asDate (holiargs.element(c+col1, r+row1)));
	    if (v.isError())
	      return Value::errorVALUE();

	    if (v.asDate( calc->doc() ).isValid())
	      holidays.append( v );
	  }
	}
    } else 
    { // no array parameter
      if (args[2].isString())
      {
        Value v( calc->conv()->asDate (args[2]) );
	if (v.isError()) 
	  return Value::errorVALUE();
	
	if (v.asDate( calc->doc() ).isValid())
	  holidays.append( v );
	
      } else
      {// isNumber 
        int hdays = calc->conv()->asInteger (args[2]).asInteger();

        if ( hdays < 0 )
	  return Value::errorVALUE();
        days = days - hdays;
      }
    }
  }
  
  //
  // count days
  //
  while(startdate != enddate)
  {
    if (startdate.dayOfWeek()>5 || holidays.contains(Value (date0.daysTo(startdate)) ))
    {
      startdate = startdate.addDays( 1*sign );
      continue;
    }
    
    startdate = startdate.addDays( 1*sign );
    days++;
  }
  return Value( days );
}

// Function: DATE2UNIX
//
// Gnumeric docs says 01/01/2000 = 946656000
// TODO:
// - create FormatType mm/dd/yyyy hh:mm:ss
// - add method tryParseDateTime
Value func_unix2date(valVector args, ValueCalc *calc, FuncExtra *)
{
    Value v( calc->conv()->asInteger( args[0] ) );
    if (v.isError())
        return v;

    QDateTime datetime;
    datetime.setTimeSpec( Qt::UTC );
    datetime.setTime_t( v.asInteger() );

    return Value( datetime.date(), calc->doc() );
}

// Function: UNIX2DATE
Value func_date2unix (valVector args, ValueCalc *calc, FuncExtra *)
{
    Value v( calc->conv()->asDate( args[0]) );
    if ( v.isError() )
        return v;

    QDateTime datetime;
    datetime.setTimeSpec( Qt::UTC );
    datetime.setDate( v.asDate( calc->doc() ) );

    return Value( static_cast<int>( datetime.toTime_t() ) );
}
