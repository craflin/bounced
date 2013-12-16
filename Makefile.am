EXTRA_DIST = TODO sample.motd
SUBDIRS = src

all: 
	@echo "-----------------------------------------------------"; \
	 echo " Bounced is now compiled."; \
	 echo ' Type "$(MAKE) install" to install Bounced as root or'; \
	 echo ' type "$(MAKE) userinstall" to install Bounced as user.'; \
	 echo "-----------------------------------------------------"

install-presetup:
	@INSTDIR="$(localstatedir)/bounced/"; \
	mkdir "$(localstatedir)" &> /dev/null; \
	mkdir "$$INSTDIR" &> /dev/null; \
	echo "$$INSTDIR" > config.dir
install: install-presetup install-setup
	@echo "-----------------------------------------------------"; \
	 echo " Bounced is now installed."; \
	 echo ' Type "bounced" to start Bounced.'; \
	 echo ' You might type "$(MAKE) clean" to clean up the package.'; \
	 echo "-----------------------------------------------------"

uninstall:
	@INSTDIR=`cat config.dir`; \
        RINSTDIR=`echo "$$INSTDIR" | sed -e s/~/\\$$HOME/1`; \
        RINSTDIR=`sh -c "echo \"$$RINSTDIR\""`; \
	if [ -d "$$RINSTDIR" ]; then \
	  rm "$$RINSTDIR/bin/bounced" "$$RINSTDIR/bounced" "$$RINSTDIR/log" "$$RINSTDIR/pid" &> /dev/null; \
	  rmdir "$$RINSTDIR/bin"; \
	  if [ -f "$$RINSTDIR/config" ] || [ -f "$$RINSTDIR/users" ] || [ -f "$$RINSTDIR/motd" ]; then \
	    echo -n "Do you want to delete your Bounced config? [no] "; \
	    read INPUT; \
	    if [ -z "$$INPUT" ]; then INPUT="no"; fi; \
	    if [ "$$INPUT" = "yes" ]; then \
	      rm "$$RINSTDIR/config" "$$RINSTDIR/config."* "$$RINSTDIR/motd" "$$RINSTDIR/users" &> /dev/null; \
	      rmdir "$$RINSTDIR"; \
	    fi; \
	  fi; \
	fi
	@echo "-----------------------------------------------------"; \
         echo " Bounced is now uninstalled."; \
	 echo "-----------------------------------------------------"

userinstall-presetup:
	@echo -n "Where do you want to install Bounced? [~/bounced] "; \
	read INSTDIR; \
	if [ -z "$$INSTDIR" ]; then INSTDIR="~/bounced"; fi; \
	RINSTDIR=`echo "$$INSTDIR" | sed -e s/~/\\$$HOME/1`; \
	RINSTDIR=`sh -c "echo \"$$RINSTDIR\""`; \
	mkdir "$$RINSTDIR" &> /dev/null; \
	mkdir "$$RINSTDIR/bin" &> /dev/null; \
	if [ ! -d "$$RINSTDIR" ] || [ ! -w "$$RINSTDIR" ]; then echo "error: Invalid install location"; exit 1; fi; \
	if [ ! -d "$$RINSTDIR/bin" ] || [ ! -w "$$RINSTDIR/bin" ]; then echo "error: Invalid install location"; exit 1; fi; \
	cp "src/bounced" "$$RINSTDIR/bin/bounced"; \
	echo -e '#!/bin/sh\ncd "'"$$RINSTDIR"'"\n./bin/bounced -c . $$*\n' > $$RINSTDIR/bounced; \
	chmod u+x "$$RINSTDIR/bounced"; \
	echo "$$INSTDIR" > config.dir
userinstall: all-recursive userinstall-presetup install-setup
	@INSTDIR=`cat config.dir`; \
	echo "-----------------------------------------------------"; \
	echo " Bounced is now installed."; \
	echo ' Type "'"$$INSTDIR"'/bounced" to start Bounced.'; \
	echo ' You might type "$(MAKE) clean" to clean up the package.'; \
	echo "-----------------------------------------------------"

install-setup:
	@INSTDIR=`cat config.dir`; \
	RINSTDIR=`echo "$$INSTDIR" | sed -e s/~/\\$$HOME/1`; \
	RINSTDIR=`sh -c "echo \"$$RINSTDIR\""`; \
	if [ ! -d "$$RINSTDIR" ] || [ ! -w "$$RINSTDIR" ]; then echo "error: Invalid install location"; exit 1; fi; \
	if [ -f "$$RINSTDIR/config" ] || [ -f "$$RINSTDIR/users" ] || [ -f "$$RINSTDIR/motd" ]; then \
	  echo "Found config files in $$INSTDIR"; \
	  echo -n "Do you want to keep your current Bounced config? [yes] "; \
	  read INPUT; \
	  if [ -z "$$INPUT" ]; then INPUT="yes"; fi; \
	  if [ "$$INPUT" != "no" ]; then NOSETUP="yes"; fi; \
	fi; \
	if [ "$$NOSETUP" != "yes" ]; then \
	  cp "sample.motd" "$$RINSTDIR/motd"; \
	  touch "$$RINSTDIR/users"; \
	  echo -n "Enter your Admin username: [Admin] "; \
	  read INPUT; \
	  if [ -z "$$INPUT" ]; then INPUT="Admin"; fi; \
	  echo 'AdminUsername "'"$$INPUT"'"' > "$$RINSTDIR/config"; \
	  echo -n "Enter your Admin password: [] "; \
	  read INPUT; \
	  echo 'AdminPassword "'"$$INPUT"'"' >> "$$RINSTDIR/config"; \
	fi; 


