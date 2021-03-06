#!/bin/sh

if [ -f "/usr/bin/sudo" ]; then
  SUDO="/usr/bin/sudo"
fi
if [ -f "../../ios-depends/build/bin/dpkg-deb" ]; then
  DPKGDEB="../../ios-depends/build/bin/dpkg-deb"
else
  DPKGDEB="dpkg-deb"
fi

PACKAGE=org.xbmc.xbmc-seatbeltunlock

VERSION=1.0
REVISION=1
ARCHIVE=${PACKAGE}_${VERSION}-${REVISION}_iphoneos-arm.deb

echo Creating $PACKAGE package version $VERSION revision $REVISION
${SUDO} rm -rf $PACKAGE
${SUDO} rm -rf $ARCHIVE

# create debian control file
mkdir -p $PACKAGE/DEBIAN
echo "Package: $PACKAGE"                                >  $PACKAGE/DEBIAN/control
echo "Priority: Extra"                                  >> $PACKAGE/DEBIAN/control
echo "Depends: coreutils, bsdiff, gawk"                 >> $PACKAGE/DEBIAN/control
echo "Name: XBMC seatbelt unlock for AppleTV 2"         >> $PACKAGE/DEBIAN/control
echo "Version: $VERSION-$REVISION"                      >> $PACKAGE/DEBIAN/control
echo "Architecture: iphoneos-arm"                       >> $PACKAGE/DEBIAN/control
echo "Description: XBMC tweeks, removes seatbelt"       >> $PACKAGE/DEBIAN/control
echo "Homepage: http://xbmc.org/"                       >> $PACKAGE/DEBIAN/control
echo "Maintainer: Scott Davilla"                        >> $PACKAGE/DEBIAN/control
echo "Author: TeamXBMC"                                 >> $PACKAGE/DEBIAN/control
echo "Section: Tweaks"                                  >> $PACKAGE/DEBIAN/control

# postinst: find lowtide/appletv, binary patch out seatbelt-profile key,
# rm/mv replace lowtide/appletv. Last step is critical as profiles are
# vnode based and there might be an existing file vnode profile.
echo "#!/bin/sh"                                        >  $PACKAGE/DEBIAN/postinst
echo "if [ \"\`uname -r\`\" = \"10.3.1\" ]; then"       >> $PACKAGE/DEBIAN/postinst
echo "  BINPATH=/Applications/Lowtide.app/Lowtide"      >> $PACKAGE/DEBIAN/postinst
echo "  echo \"Found ATV2 running ios 4.1/Lowtide\""    >> $PACKAGE/DEBIAN/postinst
echo "else"                                             >> $PACKAGE/DEBIAN/postinst
echo "  BINPATH=/Applications/AppleTV.app/AppleTV"      >> $PACKAGE/DEBIAN/postinst
echo "  echo \"Found ATV2 running ios 4.2+/AppleTV\""   >> $PACKAGE/DEBIAN/postinst
echo "fi"                                               >> $PACKAGE/DEBIAN/postinst
echo "case \`md5sum \$BINPATH | awk '{print \$1}'\` in" >> $PACKAGE/DEBIAN/postinst
echo " 12313417e3afeba6531255af58cb5283 )"              >> $PACKAGE/DEBIAN/postinst
echo "   echo \"Found 4.1 :Removing seatbelt profile key from Lowtide\"" >> $PACKAGE/DEBIAN/postinst
echo "   bspatch /Applications/Lowtide.app/Lowtide /var/tmp/Lowtide-nosb /var/tmp/12313417e3afeba6531255af58cb5283.patch" >> $PACKAGE/DEBIAN/postinst
echo "   rm /var/tmp/12313417e3afeba6531255af58cb5283.patch" >> $PACKAGE/DEBIAN/postinst
echo "   chmod 755 /var/tmp/Lowtide-nosb"               >> $PACKAGE/DEBIAN/postinst
echo "   mv -f /Applications/Lowtide.app/Lowtide /Applications/Lowtide.app/Lowtide_org" >> $PACKAGE/DEBIAN/postinst
echo "   mv /var/tmp/Lowtide-nosb /Applications/Lowtide.app/Lowtide" >> $PACKAGE/DEBIAN/postinst
echo "   killall Lowtide ;;"                            >> $PACKAGE/DEBIAN/postinst
echo " 5a28620a15c15d41e1ae836dd1f95f8d )"              >> $PACKAGE/DEBIAN/postinst
echo "   echo \"Found 4.2.1:Removing seatbelt profile key from AppleTV\"" >> $PACKAGE/DEBIAN/postinst
echo "   bspatch /Applications/AppleTV.app/AppleTV /var/tmp/AppleTV-nosb /var/tmp/5a28620a15c15d41e1ae836dd1f95f8d.patch" >> $PACKAGE/DEBIAN/postinst
echo "   rm /var/tmp/5a28620a15c15d41e1ae836dd1f95f8d.patch" >> $PACKAGE/DEBIAN/postinst
echo "   chmod 755 /var/tmp/AppleTV-nosb"               >> $PACKAGE/DEBIAN/postinst
echo "   mv -f /Applications/AppleTV.app/AppleTV /Applications/AppleTV.app/AppleTV_org" >> $PACKAGE/DEBIAN/postinst
echo "   mv /var/tmp/AppleTV-nosb /Applications/AppleTV.app/AppleTV" >> $PACKAGE/DEBIAN/postinst
echo "   killall AppleTV ;;"                            >> $PACKAGE/DEBIAN/postinst
echo " * )"                                             >> $PACKAGE/DEBIAN/postinst
echo "   echo \"Frontrow app md5sum is unknown, not patching\" ;;" >> $PACKAGE/DEBIAN/postinst
echo "esac"                                             >> $PACKAGE/DEBIAN/postinst
chmod +x $PACKAGE/DEBIAN/postinst

# create the patch directory and copy in patch
mkdir -p $PACKAGE/var/tmp
cp 12313417e3afeba6531255af58cb5283.patch               $PACKAGE/var/tmp/
cp 5a28620a15c15d41e1ae836dd1f95f8d.patch               $PACKAGE/var/tmp/

# set ownership to root:root
${SUDO} chown -R 0:0 $PACKAGE

echo Packaging $PACKAGE
# Tell tar, pax, etc. on Mac OS X 10.4+ not to archive
# extended attributes (e.g. resource forks) to ._* archive members.
# Also allows archiving and extracting actual ._* files.
export COPYFILE_DISABLE=true
export COPY_EXTENDED_ATTRIBUTES_DISABLE=true
${DPKGDEB} -b $PACKAGE $ARCHIVE
${DPKGDEB} --info $ARCHIVE
${DPKGDEB} --contents $ARCHIVE

# clean up by removing package dir
${SUDO} rm -rf $PACKAGE
