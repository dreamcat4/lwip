include Makefile.files
include Makefile.local
SRCDIR=html/
OBJDIR=obj/
LOCALDIR=`pwd`/local/
INSTALLDIR=/home/adam/public_html/lwip/
OBJFILES=$(addprefix $(OBJDIR), $(HTMLFILES))
LOCALFILES=$(addprefix $(LOCALDIR), $(HTMLFILES))
PRINTFILES=$(addsuffix -print.html, $(basename $(HTMLFILES)))


all:    clean $(HTMLFILES)

clean:
	@echo Removing HTML files...
	@rm -f $(OBJFILES) $(PRINTFILES) $(LOCALFILES)

commit:
	@echo Commiting changes
	@sh -c "cvs com obj html Makefile Makefile.files layout *m4"

install: commit all
	@echo Removing old files in $(INSTALLDIR)...
	@rm -rf $(INSTALLDIR)/*
	@echo Installing in $(INSTALLDIR)...
	sh -c "cd $(OBJDIR); cp -r . $(INSTALLDIR)"
	@echo Install done

%.html: html.m4
	@echo "Creating $@ (local)"
	@$(M4) -P -DLOCALDIR=$(LOCALDIR) -DFILE=$(basename $@) -DPRINTFILE=$(basename $@)-print.html local.m4 > $(LOCALDIR)/$@
	@echo Creating $@
	@$(M4) -P -DFILE=$(basename $@) -DPRINTFILE=$(basename $@)-print.html html.m4 > $(OBJDIR)/$@
	@echo Creating $(basename $@)-print.html
	@$(M4) -P -DFILE=$(basename $@) -DPRINTFILE=$(basename $@)-print.html print.m4 > $(OBJDIR)/$(basename $@)-print.html

