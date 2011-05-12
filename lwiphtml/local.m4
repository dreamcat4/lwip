m4_changecom(`/*', `*/')

m4_define(BASEURL, file://LOCALDIR)

m4_include(html/FILE.m4)
m4_ifdef(`CATEGORY',
`m4_include(html/CATEGORY.m4)'
,)

m4_include(layout/layout.m4)
m4_include(layout/menu.m4)


m4_include(layout/header.html)
m4_include(layout/layout.html)
m4_include(layout/footer.html)
