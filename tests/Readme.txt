ABOUT TEST FILES
---------------
In general, you do not have to look at these
None of these are actually relevant during normal runtime
To use these, additional compiler flags need to be added

Which is generally done by add -D + flagname
For example -DWORKFLOW, which triggers all #ifdef WORKFLOW snippets
It is essentialy the same as typing #define WORKFLOW inside a C++ file