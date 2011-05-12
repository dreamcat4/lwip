m4_define(MENUITEM,`
m4_ifelse(FILE, $1,<b>,<a href="BASEURL/$3">)$2`'m4_ifelse(FILE, $1,</b>,</a>)
<br>
')
