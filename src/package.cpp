/* *************************************************************************
 *  Copyright 2010 Jakob Gruber                                            *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ************************************************************************* */

#include "package.h"

#include <boost/xpressive/xpressive.hpp>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <sstream>

#include "pcursesexception.h"

using std::string;
using std::vector;
using boost::xpressive::regex_constants::icase;
using boost::xpressive::sregex;
using boost::xpressive::smatch;

Package::Package(alpm_pkg_t *pkg, alpm_db_t *localdb)
{
    alpm_pkg_t *_pkg = pkg;
    alpm_pkg_t *_localpkg = alpm_db_get_pkg(localdb, alpm_pkg_get_name(_pkg));

    _name = trimstr(alpm_pkg_get_name(_pkg));
    _url = trimstr(alpm_pkg_get_url(_pkg));
    _packager = trimstr(alpm_pkg_get_packager(_pkg));
    _desc = trimstr(alpm_pkg_get_desc(_pkg));
    _version = trimstr(alpm_pkg_get_version(_pkg));
    _dbname = trimstr(alpm_db_get_name(alpm_pkg_get_db(_pkg)));
    _builddate = alpm_pkg_get_builddate(_pkg);
    _arch = trimstr(alpm_pkg_get_arch(_pkg));

    _size = alpm_pkg_get_size(_pkg);
    _installsize = alpm_pkg_get_isize(_pkg);

    _sizestr = size2str(_size);
    _installsizestr = size2str(_installsize);

    _licenses = list2str(alpm_pkg_get_licenses(_pkg), " ");
    _groups = list2str(alpm_pkg_get_groups(_pkg), " ");

    _optdepends = deplist2str(alpm_pkg_get_optdepends(_pkg),
                              "\n            "); /* line up correctly in info pane */
    _conflicts = deplist2str(alpm_pkg_get_conflicts(_pkg), " ");
    _provides = deplist2str(alpm_pkg_get_provides(_pkg), " ");
    _replaces = deplist2str(alpm_pkg_get_replaces(_pkg), " ");
    _depends = deplist2str(alpm_pkg_get_depends(_pkg), " ");

    _signature = alpm_pkg_get_base64_sig(_pkg) ? "Yes" : "None";

    if (_localpkg == NULL) {
        _updatestate = USE_NOTINSTALLED;
    } else {
        _localversion = alpm_pkg_get_version(_localpkg);
        _updatestate = (alpm_pkg_vercmp(_version.c_str(), _localversion.c_str()) > 0) ?
                       USE_UPDATEAVAILABLE : USE_UPTODATE;

        alpm_list_t *requiredbylist = alpm_pkg_compute_requiredby(_localpkg);
        _requiredby = list2str(requiredbylist, " ");
        alpm_list_free(requiredbylist);

        alpm_list_t *optionalforlist = alpm_pkg_compute_optionalfor(_localpkg);
        _optionalfor = list2str(optionalforlist, " ");
        alpm_list_free(optionalforlist);
    }

    _reason = ((_localpkg == NULL) ? IRE_NOTINSTALLED :
               (alpm_pkg_get_reason(_localpkg) == ALPM_PKG_REASON_DEPEND) ? IRE_ASDEPS :
               IRE_EXPLICIT);
}

string Package::size2str(off_t size)
{
    std::stringstream ss;

    float fsize = size;

    int currentunit =  0;
    string units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitssize = sizeof(units) / sizeof(units[0]);

    while (fsize > 1024.0 && currentunit < unitssize - 1) {
        fsize /= 1024.0;
        currentunit++;
    }

    ss << std::fixed << std::setprecision(2) << fsize << " " << units[currentunit];

    return ss.str();
}

string Package::trimstr(const char *c) const
{
    if (c == NULL) {
        return "";
    }

    string str = c;

    /* trim string */

    size_t startpos = str.find_first_not_of(" \t\n");
    size_t endpos = str.find_last_not_of(" \t\n");

    if ((string::npos == startpos) || (string::npos == endpos)) {
        return "";
    } else {
        return str.substr(startpos, endpos - startpos + 1);
    }
}



string Package::deplist2str(alpm_list_t *l, string delim) const
{
    string res = "";
    for (alpm_list_t *deps = l; deps != NULL; deps = alpm_list_next(deps)) {
        alpm_depend_t *depend = (alpm_depend_t *)deps->data;
        res += alpm_dep_compute_string(depend);
        if (deps->next != NULL) {
            res += delim;
        }
    }
    return res;
}

string Package::list2str(alpm_list_t *l, string delim) const
{
    string s, res = "";
    for (alpm_list_t *i = l; i != NULL; i = alpm_list_next(i)) {
        s = (char *)i->data;
        if (i != l) {
            res += delim;
        }
        res += s;
    }
    return res;
}

string Package::getattr(AttributeEnum attr) const
{
    switch (attr) {
    case A_NAME:
        return getname();
    case A_VERSION:
        return getversion();
    case A_URL:
        return geturl();
    case A_REPO:
        return getrepo();
    case A_PACKAGER:
        return getpackager();
    case A_BUILDDATE:
        return getbuilddate();
    case A_INSTALLSTATE:
        return getreason();
    case A_UPDATESTATE:
        return getupdatestate();
    case A_DESC:
        return getdesc();
    case A_ARCH:
        return getarch();
    case A_LICENSES:
        return getlicenses();
    case A_GROUPS:
        return getgroups();
    case A_DEPENDS:
        return getdepends();
    case A_OPTDEPENDS:
        return getoptdepends();
    case A_CONFLICTS:
        return getconflicts();
    case A_PROVIDES:
        return getprovides();
    case A_REPLACES:
        return getreplaces();
    case A_REQUIREDBY:
        return getrequiredby();
    case A_OPTIONALFOR:
        return getoptionalfor();
    case A_SIGNATURE:
        return getsignature();
    case A_SIZE:
        return getsize();
    case A_ISIZE:
        return getisize();
    case A_NONE:
        return "";
    default:
        throw PcursesException("Invalid attribute passed.");
    }
}

off_t Package::getoffattr(AttributeEnum attr) const
{
    switch (attr) {
    case A_BUILDDATE:
        return (off_t)_builddate;
    case A_SIZE:
        return _size;
    case A_ISIZE:
        return _installsize;
    default:
        throw PcursesException("Invalid attribute passed.");
    }
}

void Package::setcolindex(int index)
{
    _colindex = index;
}

int Package::getcolindex() const
{
    return _colindex;
}

string Package::getname() const
{
    return _name;
}

string Package::getdesc() const
{
    return _desc;
}

string Package::getversion() const
{
    if (_updatestate == USE_UPDATEAVAILABLE) {
        return _version + " (local: " + _localversion + ")";
    }
    return _version;
}

string Package::getrepo() const
{
    return _dbname;
}

string Package::getreason() const
{
    switch (_reason) {
    case IRE_NOTINSTALLED:
        return "not installed";
    case IRE_EXPLICIT:
        return "explicit";
    case IRE_ASDEPS:
        return "as dependency";
    default:
        throw PcursesException("no package install reason.");
    }
}

string Package::getpackager() const
{
    return _packager;
}

string Package::geturl() const
{
    return _url;
}

string Package::getbuilddate() const
{
    string timestr = std::ctime(&_builddate);
    return timestr.substr(0, timestr.length() - 1); //remove newline
}

string Package::getarch() const
{
    return _arch;
}

string Package::getlicenses() const
{
    return _licenses;
}

string Package::getgroups() const
{
    return _groups;
}

string Package::getdepends() const
{
    return _depends;
}

string Package::getoptdepends() const
{
    return _optdepends;
}

string Package::getconflicts() const
{
    return _conflicts;
}

string Package::getprovides() const
{
    return _provides;
}

string Package::getreplaces() const
{
    return _replaces;
}

string Package::getrequiredby() const
{
    return _requiredby;
}

string Package::getoptionalfor() const
{
    return _optionalfor;
}

string Package::getsignature() const
{
    return _signature;
}

string Package::getsize() const
{
    return _sizestr;
}

string Package::getisize() const
{
    return _installsizestr;
}

string Package::getupdatestate() const
{
    switch (_updatestate) {
    case USE_NOTINSTALLED:
        return "not installed";
    case USE_UPDATEAVAILABLE:
        return "update available";
    case USE_UPTODATE:
        return "up to date";
    default:
        throw PcursesException("no package update state.");
    }
}

void Package::setop(OperationEnum oe)
{
    op = oe;
}

OperationEnum Package::getop() const
{
    return op;
}
