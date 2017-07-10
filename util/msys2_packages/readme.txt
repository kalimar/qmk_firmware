Install
MINGW_INSTALLS=mingw64 makepkg-mingw -sLf


If you get unknown keys
gmp-5.0.2.tar.bz2 ... FAILED (unknown public key 5D6D47DFDB899F46)

run 
$ gpg --recv-keys 5D6D47DFDB899F46
gpg --edit-key 5D6D47DFDB899F46
trust
Answer ultimate
quit
