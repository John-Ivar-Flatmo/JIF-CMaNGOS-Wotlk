key=$1
grep --include=\*.{c,h,cpp} -rn './' -e "$key"