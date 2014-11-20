#include "UpdateDialogGtkFactory.h"

#include "FileUtils.h"
#include "Log.h"
#include "UpdateDialog.h"
#include "StringUtils.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

class UpdateDialogGtk;

// GTK updater UI library embedded into
// the updater binary
extern unsigned char libupdatergtk_so[];
extern unsigned int libupdatergtk_so_len;

// pointers to helper functions in the GTK updater UI library
UpdateDialogGtk* (*update_dialog_gtk_new)() = 0;

#if __cplusplus >= 201103L
#define TYPEOF(x) decltype(x)
#else
#define TYPEOF(x) typeof(x)
#endif

#define BIND_FUNCTION(library,function) \
	function = reinterpret_cast<TYPEOF(function)>(dlsym(library,#function));

bool extractFileFromBinary(int fd, const void* buffer, size_t length)
{
	size_t count = write(fd,buffer,length);
	close(fd);
	return count >= length;
}

UpdateDialog* UpdateDialogGtkFactory::createDialog()
{
	char* libPath = strdup(std::string("/tmp/mendeley-libUpdaterGtk.so.XXXXXX").c_str());

	int libFd = mkostemp(libPath, O_CREAT | O_WRONLY | O_TRUNC);
	if (libFd == -1)
	{
		LOG(Warn,"Failed to create temporary file - " + std::string(strerror(errno)));
		return 0;
	}

	if (!extractFileFromBinary(libFd,libupdatergtk_so,libupdatergtk_so_len))
	{
		LOG(Warn,"Failed to load the GTK UI library - " + std::string(strerror(errno)));
		return 0;
	}

	void* gtkLib = dlopen(libPath,RTLD_LAZY);
	if (!gtkLib)
	{
		LOG(Warn,"Failed to load the GTK UI - " + std::string(dlerror()));
		return 0;
	}

	BIND_FUNCTION(gtkLib,update_dialog_gtk_new);

	FileUtils::removeFile(libPath);
	return reinterpret_cast<UpdateDialog*>(update_dialog_gtk_new());
}
