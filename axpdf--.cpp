/***********************************************************************/
/*                                                                     */
/* axpdf--.cpp: Source file for exported functions of PDF susie plugin */
/*     Written by Yak!                                                 */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/
#include <windows.h>

#include <iomanip>
#include <map>
#include <set>
#include <vector>

#include <cstdio>

#include <sys/stat.h>

#include "parser.hpp"
#include "Spi_api.h"
#define DEBUG
#include "odstream.hpp"

using yak::debug::ods;

typedef std::pair<std::string, unsigned long> Key;
typedef std::vector<char> Data;
typedef std::pair<std::vector<SPI_FILEINFO>, std::vector<Data> > Value;
std::map<Key, Value> g_cache;

// Force null terminating version of strncpy
// Return length without null terminator
int safe_strncpy(char *dest, const char *src, std::size_t n)
{
	size_t i = 0;
	while(i<n && *src) { *dest++ = *src++; ++i; }
	*dest = 0;
	return i;
}

const char* table[] = {
	"00AM",
	"PDF as an image container plugin - v0.01 Written by Yak!",
	"*.pdf",
	"PDFƒtƒ@ƒCƒ‹"
};

INT PASCAL GetPluginInfo(INT infono, LPSTR buf, INT buflen)
{
	ods << "GetPluginInfo(" << infono << ',' << buf << ',' << buflen << ')' << std::endl;
	if(0 <= infono && static_cast<size_t>(infono) < sizeof(table)/sizeof(table[0])) {
		return safe_strncpy(buf, table[infono], buflen);
	} else {
		return 0;
	}
}

static INT IsSupportedImp(LPSTR filename, LPBYTE pb)
{
	if(memcmp(pb, "%PDF-", 5) == 0) return SPI_SUPPORT_YES;
	return SPI_SUPPORT_NO;
}

INT PASCAL IsSupported(LPSTR filename, DWORD dw)
{
	ods << "IsSupported(" << filename << ',' << std::hex << std::setw(8) << std::setfill('0') << dw << ')' << std::endl;
	if(HIWORD(dw) == 0) {
		ods << "File handle" << std::endl;
		BYTE pb[2048];
		DWORD dwSize;
		ReadFile((HANDLE)dw, pb, sizeof(pb), &dwSize, NULL);
		return IsSupportedImp(filename, pb);;
	} else {
		ods << "Pointer" << std::endl; // By afx
		return IsSupportedImp(filename, (LPBYTE)dw);;
	}
	// not reached here
}

static unsigned long filesize(const char* filename)
{
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}

static Key make_key(const char* filename)
{
	return std::make_pair(std::string(filename), filesize(filename));
}

static INT SetArchiveInfo(const std::vector<SPI_FILEINFO> &v, HLOCAL *lphInf)
{
	std::size_t size = v.size() * sizeof(SPI_FILEINFO);
	std::size_t tsize = size + sizeof(SPI_FILEINFO); // for terminator
	*lphInf = LocalAlloc(LMEM_MOVEABLE, tsize);
	LPVOID pv = LocalLock(*lphInf);
	ZeroMemory(pv, tsize);
	CopyMemory(pv, &v[0], size);
	LocalUnlock(*lphInf);
	return SPI_ERR_NO_ERROR;
}

static INT CreateArchiveInfo(
	std::vector<SPI_FILEINFO> &v1,
	std::vector<Data> &v2,
	std::set<yak::pdf::indirect_ref> &set,
	const yak::pdf::pdf_reader<LPSTR> &pr,
	const yak::pdf::dictionary &dic
)
{
	using yak::pdf::has_value;
	using yak::pdf::has_key;
	using yak::pdf::name;
	using yak::pdf::array;
	using yak::pdf::object;
	using yak::pdf::dictionary;
	using yak::pdf::indirect_ref;
	using yak::pdf::stream;

	if(has_value(dic, name("Type"), name("Pages"))) {
		const array & arr = pr.resolve<array>(dic, name("Kids"));
		BOOST_FOREACH(const object &obj, arr) {
			CreateArchiveInfo(v1, v2, set, pr, pr.resolve<dictionary>(obj));
		}
	} else if(has_value(dic, name("Type"), name("Page"))) {
		if(has_key(dic, name("Resources"))) {
			std::cout << pr.resolve(dic, name("Resources")) << std::endl;
			const dictionary &res = pr.resolve<dictionary>(dic, name("Resources"));
			if(has_key(res, name("XObject"))) {
				const dictionary & xobj = pr.resolve<dictionary>(res, name("XObject"));
				BOOST_FOREACH(const dictionary::value_type &v, xobj) {
					const indirect_ref &ref = boost::get<indirect_ref>(v.second);
					if(!set.count(ref)) {
						const stream &s = pr.get<stream>(ref);
						if(has_value(s.dic, name("Type"), name("XObject")) && 
						   has_value(s.dic, name("Subtype"), name("Image")) &&
						   has_value(s.dic, name("Filter"), name("DCTDecode"))) {
							int length = pr.resolve<int>(s.dic, name("Length"));
							SPI_FILEINFO info = {
								{ 'D', 'C', 'T' },
								v1.size(),
								length,
								length
							};
							wsprintf(info.filename, "%08d.jpg", v1.size());
							v1.push_back(info);
							Data d1;
							v2.push_back(d1); v2.back().assign(s.data.begin(), s.data.begin() + length);
						}
						set.insert(ref);
					}
				}
			}
		}
	} else {
		throw yak::pdf::invalid_pdf("Unknown type in page tree");
	}
	return SPI_ERR_NO_ERROR;
}

static INT GetArchiveInfoImp(LPSTR buf, DWORD len, HLOCAL *lphInf, LPSTR filename = NULL)
{
ods << "GetArchiveInfoImp(" << std::string(buf, std::min<DWORD>(len, 1024)) << ',' << len << ',' << lphInf << (filename ? filename : "NULL") << ')' << std::endl;

	if(filename != NULL) {
		ods << "GetArchiveInfoImp - Filename specified" << std::endl;
		Key key(make_key(filename));
		if(g_cache.count(key) != 0) {
			if(lphInf) SetArchiveInfo(g_cache[key].first, lphInf);
			return SPI_ERR_NO_ERROR;
		}
	}

	try {
		yak::pdf::pdf_reader<LPSTR> pr(buf, buf+len);
		if(yak::pdf::has_key(pr.get_trailer(), yak::pdf::name("Encrypt"))) {
			throw yak::pdf::unsupported_pdf("Protected PDF is not supported.");
		}
		std::vector<SPI_FILEINFO> v1;
		std::vector<std::vector<char> > v2;
		std::set<yak::pdf::indirect_ref> set;
		CreateArchiveInfo(v1, v2, set, pr, pr.resolve<yak::pdf::dictionary>(pr.get_root(), yak::pdf::name("Pages")));
		if(lphInf) SetArchiveInfo(v1, lphInf);

		if(filename != NULL) {
			ods << "GetArchiveInfoImp - Filename specified" << std::endl;
			Key key(make_key(filename));
			g_cache[key].first.swap(v1);
			g_cache[key].second.swap(v2);
		}
	} catch(std::exception &e) {
		MessageBox(NULL, e.what(), "axpdf--.spi", MB_OK);
		return SPI_ERR_BROKEN_DATA;
	} catch(...) {
		return SPI_ERR_BROKEN_DATA;
	}

	return SPI_ERR_NO_ERROR;
}

INT PASCAL GetArchiveInfo(LPSTR buf, LONG len, UINT flag, HLOCAL *lphInf)
{
	ods << "GetArchiveInfo(" << std::string(buf, std::min<DWORD>(len, 1024)) << ',' << len << ',' << std::hex << std::setw(8) << std::setfill('0') << flag << ',' << lphInf << ')' << std::endl;
	switch(flag & 7) {
	case 0:
	  {
		using namespace boost::interprocess;
		ods << "File" << std::endl; // By afx
		file_mapping fm(buf, read_only);
		mapped_region region(fm, read_only, len);
		return GetArchiveInfoImp(static_cast<LPSTR>(region.get_address()), region.get_size(), lphInf, buf);
	  }
	case 1:
		ods << "Memory" << std::endl;
		return GetArchiveInfoImp(buf, len, lphInf);
	}
	return SPI_ERR_INTERNAL_ERROR;
}

INT PASCAL GetFileInfo(LPSTR buf, LONG len, LPSTR filename, UINT flag, SPI_FILEINFO *lpInfo)
{
	ods << "GetFileInfo(" << std::string(buf, std::min<DWORD>(len, 1024)) << ',' << len << ',' << filename << ','<< std::hex << std::setw(8) << std::setfill('0') << flag << ',' << lpInfo << ')' << std::endl;
	if(flag & 128) {
		ods << "Case-insensitive" << std::endl; // By afx
	} else {
		ods << "Case-sensitive" << std::endl;
	}
	switch(flag & 7) {
	case 0:
		ods << "File" << std::endl; // By afx
		break;
	case 1:
		ods << "Memory" << std::endl;
	}
	HLOCAL hInfo;
	if(GetArchiveInfo(buf, len, flag&7, &hInfo) == SPI_ERR_NO_ERROR) {
		int (WINAPI *compare)(LPCSTR, LPCSTR) = flag&7 ? lstrcmpi : lstrcmp;
		std::size_t size = LocalSize(hInfo);
		SPI_FILEINFO *p = static_cast<SPI_FILEINFO*>(LocalLock(hInfo)), *cur = p;
		std::size_t count = size / sizeof(SPI_FILEINFO);
		while(cur < p + count && *static_cast<char*>(static_cast<void*>(cur)) != '\0') {
			if(!compare(cur->filename, filename)) {
				CopyMemory(lpInfo, cur, sizeof(SPI_FILEINFO));
				LocalUnlock(hInfo);
				LocalFree(hInfo);
				return SPI_ERR_NO_ERROR;
			}
			++cur;
		}
		LocalUnlock(hInfo);
		LocalFree(hInfo);
	}
	return SPI_ERR_INTERNAL_ERROR;
}

INT PASCAL GetFile(LPSTR buf, LONG len, LPSTR dest, UINT flag, FARPROC prgressCallback, LONG lData)
{
	ods << "GetFile(" << ',' << len << ',' << ',' << flag << ',' << prgressCallback << ',' << lData << ')' << std::endl;
	switch(flag & 7) {
	case 0:
	  {
		ods << "Source is file" << std::endl; // By afx
		break;
	  }
	case 1:
		ods << "Source is memory" << std::endl;
		return SPI_ERR_NOT_IMPLEMENTED; // We cannot handle this case.
	}
	switch((flag>>8) & 7) {
	case 0:
		ods << "Destination is file: " << dest << std::endl;
		break;
	case 1:
		ods << "Destination is memory: " << reinterpret_cast<void*>(dest) << std::endl; // By afx
		break;
	}
	if(GetArchiveInfo(buf, len, flag&7, 0) == SPI_ERR_NO_ERROR) {
		ods << "GetFile(): GetArvhiveInfo() returned" << std::endl;
		Key key(make_key(buf));
		if(g_cache.count(key) == 0) {
			ods << "GetFile(): " << buf << " not cached" << std::endl;
			return SPI_ERR_INTERNAL_ERROR;
		}
		Value &value = g_cache[key];
		std::size_t size = value.first.size();
		for(std::size_t i = 0; i < size; ++i) {
			if(value.first[i].position == std::size_t(len)) {
				ods << "GetFile(): position found" << std::endl;
				if((flag>>8) & 7) { // memory
					if(dest) {
						ods << "GetFile(): size: " << value.second[i].size() << " head: " << value.second[i][0] << value.second[i][1] << value.second[i][2] << value.second[i][3] << std::endl;
						HANDLE *phResult = reinterpret_cast<HANDLE*>(dest);
						*phResult = LocalAlloc(LMEM_MOVEABLE, value.second[i].size());
						void* p = LocalLock(*phResult);
						CopyMemory(p, &value.second[i][0], value.second[i].size());
						LocalUnlock(*phResult);
						return SPI_ERR_NO_ERROR;
					}
					return SPI_ERR_INTERNAL_ERROR;
				} else { // file
					std::string s(dest);
					s += '\\';
					s += value.first[i].filename;
					FILE *fp = std::fopen(s.c_str(), "wb");
					fwrite(&value.second[i][0], value.second[i].size(), 1, fp);
					fclose(fp);
					return SPI_ERR_NO_ERROR;
				}
			}
		}
	}
	ods << "GetFile(): position not found" << std::endl;
	return SPI_ERR_INTERNAL_ERROR;
}
