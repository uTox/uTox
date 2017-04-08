# Changelog

## [Unreleased](https://github.com/uTox/uTox/tree/HEAD)

[Full Changelog](https://github.com/uTox/uTox/compare/v0.11.1...HEAD)

**Implemented enhancements:**

- \[feature request\] Ability to change «nospam» part of ToxID from µTox UI. [\#406](https://github.com/uTox/uTox/issues/406)

**Fixed bugs:**

- filetransfer resume has UI errors [\#620](https://github.com/uTox/uTox/issues/620)
- Segfault on any contact going offline \(current nightly, Linux\) [\#616](https://github.com/uTox/uTox/issues/616)
- File transfers crash, 0.11.1 [\#601](https://github.com/uTox/uTox/issues/601)
- Tox ID isn't displayed [\#592](https://github.com/uTox/uTox/issues/592)
- time display is missing the last digit [\#581](https://github.com/uTox/uTox/issues/581)
- UI glitch with password field [\#570](https://github.com/uTox/uTox/issues/570)
- GUI freeze when I try to change network settings 0.9.7 [\#544](https://github.com/uTox/uTox/issues/544)
- Problems in the Video Preview [\#251](https://github.com/uTox/uTox/issues/251)

**Closed issues:**

- segfault on exporting changelog [\#630](https://github.com/uTox/uTox/issues/630)
- Linker warning, related to forward declares? [\#628](https://github.com/uTox/uTox/issues/628)
- uTox is prevented from starting on windows with some McAffee thingy installed [\#621](https://github.com/uTox/uTox/issues/621)
- uTox making random noises [\#585](https://github.com/uTox/uTox/issues/585)
- chat history is lost after changing network settings [\#582](https://github.com/uTox/uTox/issues/582)
- Wrong text when hovering on buttons [\#576](https://github.com/uTox/uTox/issues/576)
- \[binaries\] Linux static x32 binary is not static and not x32 [\#567](https://github.com/uTox/uTox/issues/567)
- Language selector shows multiple options for english [\#563](https://github.com/uTox/uTox/issues/563)
- DPI and Can't Connect [\#459](https://github.com/uTox/uTox/issues/459)

**Merged pull requests:**

- Merge nospam into develop. \(Merge conflict with changes\) [\#635](https://github.com/uTox/uTox/pull/635) ([GrayHatter](https://github.com/GrayHatter))
- Don't segv when exporting chat history [\#631](https://github.com/uTox/uTox/pull/631) ([GrayHatter](https://github.com/GrayHatter))
- Change some variables over to being declared rather than defined in .h files. [\#629](https://github.com/uTox/uTox/pull/629) ([robinlinden](https://github.com/robinlinden))
- Allow uTox to check for updates [\#627](https://github.com/uTox/uTox/pull/627) ([GrayHatter](https://github.com/GrayHatter))
- Reduce and make includes explicit. [\#626](https://github.com/uTox/uTox/pull/626) ([robinlinden](https://github.com/robinlinden))
- Build fixes for windows [\#624](https://github.com/uTox/uTox/pull/624) ([GrayHatter](https://github.com/GrayHatter))
- WIP commit for fixing file sending on windows [\#622](https://github.com/uTox/uTox/pull/622) ([GrayHatter](https://github.com/GrayHatter))
- Add .gitattribute file [\#619](https://github.com/uTox/uTox/pull/619) ([GrayHatter](https://github.com/GrayHatter))
- WinXP support [\#618](https://github.com/uTox/uTox/pull/618) ([GrayHatter](https://github.com/GrayHatter))
- separated out logging from main.h [\#617](https://github.com/uTox/uTox/pull/617) ([cebe](https://github.com/cebe))
- Make UI less dependent on the rest of uTox. [\#610](https://github.com/uTox/uTox/pull/610) ([robinlinden](https://github.com/robinlinden))
- Friend request improvements [\#608](https://github.com/uTox/uTox/pull/608) ([endoffile78](https://github.com/endoffile78))
- Add sign-release.sh and checksum.sh [\#607](https://github.com/uTox/uTox/pull/607) ([endoffile78](https://github.com/endoffile78))
- enable address sanitizer in cmake debug builds [\#606](https://github.com/uTox/uTox/pull/606) ([cebe](https://github.com/cebe))
- port the add\_cflags makefile function from toxcore [\#605](https://github.com/uTox/uTox/pull/605) ([cebe](https://github.com/cebe))
- Fix avatars, and mkdir for posix [\#604](https://github.com/uTox/uTox/pull/604) ([GrayHatter](https://github.com/GrayHatter))
- Use find\_package to find X11 and Xrender [\#599](https://github.com/uTox/uTox/pull/599) ([endoffile78](https://github.com/endoffile78))
- Fix defects detected by coverity [\#596](https://github.com/uTox/uTox/pull/596) ([endoffile78](https://github.com/endoffile78))
- fix utox clobbering chatlogs [\#595](https://github.com/uTox/uTox/pull/595) ([GrayHatter](https://github.com/GrayHatter))
- Delete .clang-format [\#594](https://github.com/uTox/uTox/pull/594) ([GrayHatter](https://github.com/GrayHatter))
- configure version in one place [\#593](https://github.com/uTox/uTox/pull/593) ([cebe](https://github.com/cebe))
- fixes build for linux [\#591](https://github.com/uTox/uTox/pull/591) ([cebe](https://github.com/cebe))
- Refactor postmessages\(\) [\#590](https://github.com/uTox/uTox/pull/590) ([GrayHatter](https://github.com/GrayHatter))
- Fix uTox not using the MAIN\_HEIGHT & WIDTH defines. [\#579](https://github.com/uTox/uTox/pull/579) ([robinlinden](https://github.com/robinlinden))
- Apply irungentoo's fixes for the XP build and AUDIO\_FILTERING [\#578](https://github.com/uTox/uTox/pull/578) ([robinlinden](https://github.com/robinlinden))
- Fix switches [\#577](https://github.com/uTox/uTox/pull/577) ([cebe](https://github.com/cebe))
- Split CMakeLists.txt into multiple files [\#575](https://github.com/uTox/uTox/pull/575) ([endoffile78](https://github.com/endoffile78))
- Fix friend notification sounds [\#573](https://github.com/uTox/uTox/pull/573) ([GrayHatter](https://github.com/GrayHatter))
- Fixed Hanging GUI when changing network settings [\#569](https://github.com/uTox/uTox/pull/569) ([cebe](https://github.com/cebe))
- Fix names of links to Linux/Posix builds. [\#568](https://github.com/uTox/uTox/pull/568) ([robinlinden](https://github.com/robinlinden))
- Fix when and where we fflush. [\#566](https://github.com/uTox/uTox/pull/566) ([robinlinden](https://github.com/robinlinden))
- Update BUILD.md [\#565](https://github.com/uTox/uTox/pull/565) ([cebe](https://github.com/cebe))
- Fix language-selection dropdown having multiple English languages and none of others. [\#564](https://github.com/uTox/uTox/pull/564) ([robinlinden](https://github.com/robinlinden))

## [v0.11.1](https://github.com/uTox/uTox/tree/v0.11.1) (2016-12-21)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.11.0...v0.11.1)

**Closed issues:**

- passing a null pointer for dereference, presumably not intentional [\#556](https://github.com/uTox/uTox/issues/556)
- GPG signatures for source validation [\#502](https://github.com/uTox/uTox/issues/502)

**Merged pull requests:**

- Rename onpress to on\_mup [\#589](https://github.com/uTox/uTox/pull/589) ([GrayHatter](https://github.com/GrayHatter))
- Hotfix: drop a bad attempt at secure erase [\#562](https://github.com/uTox/uTox/pull/562) ([GrayHatter](https://github.com/GrayHatter))
- Hotfix: drop a bad attempt at secure erase [\#561](https://github.com/uTox/uTox/pull/561) ([GrayHatter](https://github.com/GrayHatter))
- v0.11.1 to master [\#560](https://github.com/uTox/uTox/pull/560) ([GrayHatter](https://github.com/GrayHatter))
- Write '\0' instead of 0 in ft\_decon\_resumeable [\#559](https://github.com/uTox/uTox/pull/559) ([robinlinden](https://github.com/robinlinden))
- Updated changelog for 0.11.1 [\#557](https://github.com/uTox/uTox/pull/557) ([robinlinden](https://github.com/robinlinden))
- Fix Windows native\_get\_file folder creation. [\#552](https://github.com/uTox/uTox/pull/552) ([robinlinden](https://github.com/robinlinden))
- Move chatlog-related functions to their own file. [\#551](https://github.com/uTox/uTox/pull/551) ([robinlinden](https://github.com/robinlinden))
- Update OS X plist to use the correct version number. [\#548](https://github.com/uTox/uTox/pull/548) ([robinlinden](https://github.com/robinlinden))
- Update tools [\#547](https://github.com/uTox/uTox/pull/547) ([endoffile78](https://github.com/endoffile78))
- Fix segv in ft\_find\_resumeable in file\_transfer.c on Windows [\#546](https://github.com/uTox/uTox/pull/546) ([robinlinden](https://github.com/robinlinden))
- Force toxcore \>= v0.1 [\#542](https://github.com/uTox/uTox/pull/542) ([GrayHatter](https://github.com/GrayHatter))
- Version tick [\#541](https://github.com/uTox/uTox/pull/541) ([endoffile78](https://github.com/endoffile78))
- Add skel checklist for new versions [\#523](https://github.com/uTox/uTox/pull/523) ([GrayHatter](https://github.com/GrayHatter))

## [v0.11.0](https://github.com/uTox/uTox/tree/v0.11.0) (2016-12-13)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.8...v0.11.0)

**Implemented enhancements:**

- "Show" button remains even when it has performed its function [\#440](https://github.com/uTox/uTox/issues/440)
- Auto accept files portable mode [\#402](https://github.com/uTox/uTox/issues/402)
- Group chats do not call notify\(\) to generate a platform specific notification. [\#272](https://github.com/uTox/uTox/issues/272)

**Fixed bugs:**

- "Show" button remains even when it has performed its function [\#440](https://github.com/uTox/uTox/issues/440)
- The "Show" button is on the title it meant to be next to [\#439](https://github.com/uTox/uTox/issues/439)
- uTox segfaults if the "end" key is pressed when the message input box is empty [\#417](https://github.com/uTox/uTox/issues/417)
- Patch: utox w/xlib not reaping zombie procs from openurl\(\)/xdg-open [\#412](https://github.com/uTox/uTox/issues/412)
- Group chats do not call notify\\(\\) to generate a platform specific notification. [\#272](https://github.com/uTox/uTox/issues/272)

**Closed issues:**

- compiler error on \(Arch\)linux [\#525](https://github.com/uTox/uTox/issues/525)
- Utox it's not autoupdating ? [\#520](https://github.com/uTox/uTox/issues/520)
- utox won't start - Trace/breakpoint trap / Illegal instruction / Segmentation fault [\#498](https://github.com/uTox/uTox/issues/498)
- Themes are not being saved [\#471](https://github.com/uTox/uTox/issues/471)
- friend\_meta\_data\_read is missing a null check after calloc [\#453](https://github.com/uTox/uTox/issues/453)
- Reason to have pulse audio DONT\_MOVE flag [\#448](https://github.com/uTox/uTox/issues/448)
- "Audio/Video" tab button is too fat [\#441](https://github.com/uTox/uTox/issues/441)
- Manpage gives different Git repository [\#438](https://github.com/uTox/uTox/issues/438)
- FIX headers, and includes. [\#436](https://github.com/uTox/uTox/issues/436)
- uTow not starting ? \(Unable to get saved avatar from disk for friend ?\) [\#418](https://github.com/uTox/uTox/issues/418)
- Can't add friends to group. [\#416](https://github.com/uTox/uTox/issues/416)
- startup regression \(reading config\) [\#413](https://github.com/uTox/uTox/issues/413)
- Multiple Instances running causes conflict [\#407](https://github.com/uTox/uTox/issues/407)
- Windows version uTox compile problems. [\#405](https://github.com/uTox/uTox/issues/405)
- User Interface Settings: Non-english labels overlap [\#346](https://github.com/uTox/uTox/issues/346)
- send typing notification not remembered [\#320](https://github.com/uTox/uTox/issues/320)
- Add support for the ToxMe https lookup api [\#145](https://github.com/uTox/uTox/issues/145)

**Merged pull requests:**

- v0.11.0 to master [\#540](https://github.com/uTox/uTox/pull/540) ([GrayHatter](https://github.com/GrayHatter))
- Version 0.9.8 =\> 0.11.0 [\#539](https://github.com/uTox/uTox/pull/539) ([GrayHatter](https://github.com/GrayHatter))
- Fix autoaccept [\#538](https://github.com/uTox/uTox/pull/538) ([endoffile78](https://github.com/endoffile78))
- Fix saving and loading of friend's metadata [\#536](https://github.com/uTox/uTox/pull/536) ([GrayHatter](https://github.com/GrayHatter))
- Minor refactor to split off the uTox code from the Toxcore code [\#534](https://github.com/uTox/uTox/pull/534) ([GrayHatter](https://github.com/GrayHatter))
- Add more things to .editorconfig [\#533](https://github.com/uTox/uTox/pull/533) ([robinlinden](https://github.com/robinlinden))
- Fix warnings, cleanup, and refactoring. [\#532](https://github.com/uTox/uTox/pull/532) ([robinlinden](https://github.com/robinlinden))
- Fix chat logging [\#531](https://github.com/uTox/uTox/pull/531) ([robinlinden](https://github.com/robinlinden))
- Add .editorconfig [\#530](https://github.com/uTox/uTox/pull/530) ([endoffile78](https://github.com/endoffile78))
- Fix warnings [\#527](https://github.com/uTox/uTox/pull/527) ([endoffile78](https://github.com/endoffile78))
- Fix POSIX native\_get\_file. [\#526](https://github.com/uTox/uTox/pull/526) ([robinlinden](https://github.com/robinlinden))
- Refactor commands [\#524](https://github.com/uTox/uTox/pull/524) ([endoffile78](https://github.com/endoffile78))
- Remove avatar functions from main.c [\#522](https://github.com/uTox/uTox/pull/522) ([endoffile78](https://github.com/endoffile78))
- Add documentation to functions in main.h [\#521](https://github.com/uTox/uTox/pull/521) ([endoffile78](https://github.com/endoffile78))
- Merge branch [\#519](https://github.com/uTox/uTox/pull/519) ([endoffile78](https://github.com/endoffile78))
- Update BUILD.md [\#517](https://github.com/uTox/uTox/pull/517) ([cebe](https://github.com/cebe))
- Update .gitignore [\#514](https://github.com/uTox/uTox/pull/514) ([endoffile78](https://github.com/endoffile78))
- Add ability to change nospam [\#513](https://github.com/uTox/uTox/pull/513) ([endoffile78](https://github.com/endoffile78))
- Add xlib native\_get\_file option to delete file. [\#512](https://github.com/uTox/uTox/pull/512) ([robinlinden](https://github.com/robinlinden))
- Lots of native function things. [\#511](https://github.com/uTox/uTox/pull/511) ([robinlinden](https://github.com/robinlinden))
- Avatar refactor [\#510](https://github.com/uTox/uTox/pull/510) ([robinlinden](https://github.com/robinlinden))
- Better cmake && extern to avoid a segfault on windows [\#508](https://github.com/uTox/uTox/pull/508) ([GrayHatter](https://github.com/GrayHatter))
- Revert "Add debug option to CMakeLists.txt" [\#506](https://github.com/uTox/uTox/pull/506) ([endoffile78](https://github.com/endoffile78))
- Change type of nick in edits.c [\#505](https://github.com/uTox/uTox/pull/505) ([endoffile78](https://github.com/endoffile78))
- Fix warnings [\#504](https://github.com/uTox/uTox/pull/504) ([endoffile78](https://github.com/endoffile78))
- Add debug option to CMakeLists.txt [\#503](https://github.com/uTox/uTox/pull/503) ([endoffile78](https://github.com/endoffile78))
- Fixed order of things in the A&V settings. [\#501](https://github.com/uTox/uTox/pull/501) ([robinlinden](https://github.com/robinlinden))
- OSX building and travis [\#500](https://github.com/uTox/uTox/pull/500) ([endoffile78](https://github.com/endoffile78))
- Fix groupchat names [\#499](https://github.com/uTox/uTox/pull/499) ([robinlinden](https://github.com/robinlinden))
- Add install rules to CMakeLists.txt [\#496](https://github.com/uTox/uTox/pull/496) ([endoffile78](https://github.com/endoffile78))
- Clean up and refactor UI code [\#492](https://github.com/uTox/uTox/pull/492) ([robinlinden](https://github.com/robinlinden))
- Added Native file functions to Windows [\#490](https://github.com/uTox/uTox/pull/490) ([GrayHatter](https://github.com/GrayHatter))
- Update src/cocoa/main.m to use new debug enum names [\#489](https://github.com/uTox/uTox/pull/489) ([robinlinden](https://github.com/robinlinden))
- fix the icons in windows [\#488](https://github.com/uTox/uTox/pull/488) ([GrayHatter](https://github.com/GrayHatter))
- Fixed broken filter friend button [\#486](https://github.com/uTox/uTox/pull/486) ([GrayHatter](https://github.com/GrayHatter))
- fix travis error on prebuild [\#485](https://github.com/uTox/uTox/pull/485) ([GrayHatter](https://github.com/GrayHatter))
- Fix Buttons on windows [\#484](https://github.com/uTox/uTox/pull/484) ([GrayHatter](https://github.com/GrayHatter))
- UI code cleanup [\#483](https://github.com/uTox/uTox/pull/483) ([robinlinden](https://github.com/robinlinden))
- Clicking "show profile password" button hides it. [\#482](https://github.com/uTox/uTox/pull/482) ([robinlinden](https://github.com/robinlinden))
- Fix avatars [\#481](https://github.com/uTox/uTox/pull/481) ([endoffile78](https://github.com/endoffile78))
- Fix segfault in image\_free [\#480](https://github.com/uTox/uTox/pull/480) ([endoffile78](https://github.com/endoffile78))
- Fixing more warnings [\#479](https://github.com/uTox/uTox/pull/479) ([robinlinden](https://github.com/robinlinden))
- Stop utox from segfaulting [\#477](https://github.com/uTox/uTox/pull/477) ([endoffile78](https://github.com/endoffile78))
- Update BUILD.md and INSTALL.md [\#476](https://github.com/uTox/uTox/pull/476) ([endoffile78](https://github.com/endoffile78))
- Fixing warnings [\#475](https://github.com/uTox/uTox/pull/475) ([robinlinden](https://github.com/robinlinden))
- Fix CMake and Travis on Win [\#470](https://github.com/uTox/uTox/pull/470) ([GrayHatter](https://github.com/GrayHatter))
- Removed superfluous ../main.h includes in general and xlib files [\#468](https://github.com/uTox/uTox/pull/468) ([robinlinden](https://github.com/robinlinden))
- Limited width of audio-video tab in settings. \(fixes \#441\) [\#467](https://github.com/uTox/uTox/pull/467) ([robinlinden](https://github.com/robinlinden))
- Replace native\_load\_data and native\_save\_data with native\_get\_file [\#466](https://github.com/uTox/uTox/pull/466) ([endoffile78](https://github.com/endoffile78))
- Ignore cmake files [\#464](https://github.com/uTox/uTox/pull/464) ([endoffile78](https://github.com/endoffile78))
- Remove language strings not in use. [\#463](https://github.com/uTox/uTox/pull/463) ([robinlinden](https://github.com/robinlinden))
- Add null checks after calloc and malloc [\#457](https://github.com/uTox/uTox/pull/457) ([endoffile78](https://github.com/endoffile78))
- Friend list now responds to mup \(from mdown\) [\#455](https://github.com/uTox/uTox/pull/455) ([GrayHatter](https://github.com/GrayHatter))
- On going Refactor [\#454](https://github.com/uTox/uTox/pull/454) ([GrayHatter](https://github.com/GrayHatter))
- Updated russian translation [\#451](https://github.com/uTox/uTox/pull/451) ([katyo](https://github.com/katyo))
- Updated changelog [\#447](https://github.com/uTox/uTox/pull/447) ([endoffile78](https://github.com/endoffile78))
- .travis Fix travis builds to work with the new toktok/toxcore [\#446](https://github.com/uTox/uTox/pull/446) ([GrayHatter](https://github.com/GrayHatter))
- Notify the user when a friend comes online and goes offline [\#444](https://github.com/uTox/uTox/pull/444) ([endoffile78](https://github.com/endoffile78))
- Update the manual page [\#443](https://github.com/uTox/uTox/pull/443) ([tsudoko](https://github.com/tsudoko))
- Add header guards [\#442](https://github.com/uTox/uTox/pull/442) ([endoffile78](https://github.com/endoffile78))
- Make Solarized themes selectable with --theme [\#432](https://github.com/uTox/uTox/pull/432) ([tsudoko](https://github.com/tsudoko))
- Use debug\_error\(\) for getopt errors [\#431](https://github.com/uTox/uTox/pull/431) ([tsudoko](https://github.com/tsudoko))
- Add invite command [\#430](https://github.com/uTox/uTox/pull/430) ([endoffile78](https://github.com/endoffile78))
- Make the "yes" icon not inverted [\#429](https://github.com/uTox/uTox/pull/429) ([tsudoko](https://github.com/tsudoko))
- Fix switches [\#426](https://github.com/uTox/uTox/pull/426) ([tsudoko](https://github.com/tsudoko))
- Fix disabled hover color in themes [\#425](https://github.com/uTox/uTox/pull/425) ([tsudoko](https://github.com/tsudoko))
- add Solarized colour schemes [\#423](https://github.com/uTox/uTox/pull/423) ([ninedotnine](https://github.com/ninedotnine))
- Update German translation [\#409](https://github.com/uTox/uTox/pull/409) ([sfan5](https://github.com/sfan5))
- Decline unwanted calls [\#404](https://github.com/uTox/uTox/pull/404) ([Encrypt](https://github.com/Encrypt))

## [v0.9.8](https://github.com/uTox/uTox/tree/v0.9.8) (2016-07-29)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.7...v0.9.8)

**Closed issues:**

- Cross traffic between chat threads [\#401](https://github.com/uTox/uTox/issues/401)

## [v0.9.7](https://github.com/uTox/uTox/tree/v0.9.7) (2016-07-26)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.4...v0.9.7)

**Implemented enhancements:**

- /tools/logs\_to\_plaintext.c needs to be updated [\#309](https://github.com/uTox/uTox/issues/309)

**Fixed bugs:**

- Windows XP version error \("RegDeleteKeyValueW" and ADVAPI32.dll\) [\#349](https://github.com/uTox/uTox/issues/349)
- /tools/logs\\_to\\_plaintext.c needs to be updated [\#309](https://github.com/uTox/uTox/issues/309)

**Closed issues:**

- Very network inefficient [\#389](https://github.com/uTox/uTox/issues/389)
- Very network inefficient [\#387](https://github.com/uTox/uTox/issues/387)
- Offline messages - security [\#386](https://github.com/uTox/uTox/issues/386)
- Chat text appears red when ending with \< [\#385](https://github.com/uTox/uTox/issues/385)
- Corrupting history? [\#380](https://github.com/uTox/uTox/issues/380)
- Avatar doesn't stick [\#375](https://github.com/uTox/uTox/issues/375)
- properly handle non-multidevice nodes [\#371](https://github.com/uTox/uTox/issues/371)
- Noise cancellation [\#363](https://github.com/uTox/uTox/issues/363)
- Antox closes when sending file with UTOX. [\#361](https://github.com/uTox/uTox/issues/361)
- Instant crash on start \(windows\) [\#343](https://github.com/uTox/uTox/issues/343)
- uTox \(v0.8.2 - 0.9.1\) automatically exit under Win10 [\#341](https://github.com/uTox/uTox/issues/341)

**Merged pull requests:**

- Converted all yes / no dropdowns to switches [\#397](https://github.com/uTox/uTox/pull/397) ([Encrypt](https://github.com/Encrypt))
- Update fr.h [\#391](https://github.com/uTox/uTox/pull/391) ([Encrypt](https://github.com/Encrypt))
- Update German translations [\#383](https://github.com/uTox/uTox/pull/383) ([sfan5](https://github.com/sfan5))
- Adding a changelog to the project. [\#382](https://github.com/uTox/uTox/pull/382) ([Encrypt](https://github.com/Encrypt))
- Added settings for group chats [\#378](https://github.com/uTox/uTox/pull/378) ([endoffile78](https://github.com/endoffile78))
- Update cygwin-compile.sh [\#374](https://github.com/uTox/uTox/pull/374) ([endoffile78](https://github.com/endoffile78))
- Add files created by editors to .gitignore [\#369](https://github.com/uTox/uTox/pull/369) ([endoffile78](https://github.com/endoffile78))
- ui.c new button for chat log export [\#367](https://github.com/uTox/uTox/pull/367) ([utoxxer](https://github.com/utoxxer))
- Chat log export button [\#366](https://github.com/uTox/uTox/pull/366) ([utoxxer](https://github.com/utoxxer))
- Update fr.h [\#365](https://github.com/uTox/uTox/pull/365) ([pthevenet](https://github.com/pthevenet))
- Re-introduced audio filtering [\#364](https://github.com/uTox/uTox/pull/364) ([Encrypt](https://github.com/Encrypt))
- Update German translation [\#360](https://github.com/uTox/uTox/pull/360) ([sfan5](https://github.com/sfan5))
- version typo fix [\#345](https://github.com/uTox/uTox/pull/345) ([felix-salfelder](https://github.com/felix-salfelder))

## [v0.9.4](https://github.com/uTox/uTox/tree/v0.9.4) (2016-05-25)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.3...v0.9.4)

**Fixed bugs:**

- uTox not responding after it connects [\#334](https://github.com/uTox/uTox/issues/334)

## [v0.9.3](https://github.com/uTox/uTox/tree/v0.9.3) (2016-05-23)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.1...v0.9.3)

**Fixed bugs:**

- \[end\] key goes to end of input, not end of line [\#338](https://github.com/uTox/uTox/issues/338)

**Closed issues:**

- uTox crashes upon connecting to friend \(alpha 0.9.1 build downloaded tonight, immediate issue upon post update execution\) [\#337](https://github.com/uTox/uTox/issues/337)

**Merged pull requests:**

- Implement inline video in cocoa [\#340](https://github.com/uTox/uTox/pull/340) ([stal888](https://github.com/stal888))

## [v0.9.1](https://github.com/uTox/uTox/tree/v0.9.1) (2016-05-21)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.9.0...v0.9.1)

## [v0.9.0](https://github.com/uTox/uTox/tree/v0.9.0) (2016-05-21)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.8.2...v0.9.0)

**Implemented enhancements:**

- Add day changes to messages [\#327](https://github.com/uTox/uTox/issues/327)
- Allow switching between audio only/video call without having to restart the call [\#95](https://github.com/uTox/uTox/issues/95)
- Feature: Showing full date in message log when it's a different day [\#65](https://github.com/uTox/uTox/issues/65)

**Fixed bugs:**

- Opening links is broken [\#330](https://github.com/uTox/uTox/issues/330)
- The baloon is gone [\#319](https://github.com/uTox/uTox/issues/319)
- Clicking/dragging empty space near a hyperlink opens it [\#318](https://github.com/uTox/uTox/issues/318)
- Offline messages are delivered in wrong order [\#316](https://github.com/uTox/uTox/issues/316)
- Super small window on start [\#314](https://github.com/uTox/uTox/issues/314)
- utox kills itself trying to malloc insane amount of memory at startup [\#313](https://github.com/uTox/uTox/issues/313)
- Groupchat window can't be scrolled in any way [\#298](https://github.com/uTox/uTox/issues/298)
- DPI UI error in Win Vista x64 [\#224](https://github.com/uTox/uTox/issues/224)
- utox hogs the first and current sound cards all the time [\#223](https://github.com/uTox/uTox/issues/223)
- when got a message, uTox has no tip audio, how to fix? [\#207](https://github.com/uTox/uTox/issues/207)
- uTox re-encodes PNG files when setting an avatar [\#179](https://github.com/uTox/uTox/issues/179)
- SegFault on Connected to DHT [\#159](https://github.com/uTox/uTox/issues/159)

**Closed issues:**

- blue text and dark theme [\#315](https://github.com/uTox/uTox/issues/315)
- Recent utox versions re-send entire message history every time a friend comes online [\#312](https://github.com/uTox/uTox/issues/312)
- Updating to 0.8.1 causes broken message windows on first run [\#310](https://github.com/uTox/uTox/issues/310)
- Beep on new message. [\#233](https://github.com/uTox/uTox/issues/233)
- Switching video to "none" while previewing confuses utox [\#221](https://github.com/uTox/uTox/issues/221)
- Offline message [\#195](https://github.com/uTox/uTox/issues/195)
- uTox does not compile in Gentoo [\#164](https://github.com/uTox/uTox/issues/164)

**Merged pull requests:**

- Version v0.9.0 SRSLY? [\#333](https://github.com/uTox/uTox/pull/333) ([GrayHatter](https://github.com/GrayHatter))
- changed dark theme pending message color for better contrast [\#317](https://github.com/uTox/uTox/pull/317) ([cebe](https://github.com/cebe))

## [v0.8.2](https://github.com/uTox/uTox/tree/v0.8.2) (2016-05-06)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.8.1...v0.8.2)

**Fixed bugs:**

- Version 0.8.1 doesn't always give flash alert or message bubble to alert new message [\#307](https://github.com/uTox/uTox/issues/307)
- Text selection in groupchats is broken [\#305](https://github.com/uTox/uTox/issues/305)
- Names aren't displayed in groupchats [\#297](https://github.com/uTox/uTox/issues/297)
- Bugs with non-Latin filenames [\#291](https://github.com/uTox/uTox/issues/291)
- Unable to initiate Group Chats [\#255](https://github.com/uTox/uTox/issues/255)
- message notifications no longer work on linux [\#205](https://github.com/uTox/uTox/issues/205)
- File transfer icon looks bad [\#127](https://github.com/uTox/uTox/issues/127)
- Username doesn't become blue when using /me in group chat [\#58](https://github.com/uTox/uTox/issues/58)

**Closed issues:**

- eye-tearing antialiased font [\#311](https://github.com/uTox/uTox/issues/311)
- uTox tox save reverting to a version from months back [\#202](https://github.com/uTox/uTox/issues/202)
- UI looks bad on some DPI settings [\#126](https://github.com/uTox/uTox/issues/126)

**Merged pull requests:**

- Update German translation [\#308](https://github.com/uTox/uTox/pull/308) ([sfan5](https://github.com/sfan5))
- Messages queue [\#295](https://github.com/uTox/uTox/pull/295) ([GrayHatter](https://github.com/GrayHatter))

## [v0.8.1](https://github.com/uTox/uTox/tree/v0.8.1) (2016-04-29)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.8.0...v0.8.1)

**Implemented enhancements:**

- Messages.c Refactor [\#128](https://github.com/uTox/uTox/issues/128)
- \[Feature request\] Meta-contacts \(unite/group several contacs in one\) [\#97](https://github.com/uTox/uTox/issues/97)
- Implement Tox Client Standard concerning the avatars [\#45](https://github.com/uTox/uTox/issues/45)
- Save Auto Accept settings for uTox [\#30](https://github.com/uTox/uTox/issues/30)

**Fixed bugs:**

- Group chats [\#293](https://github.com/uTox/uTox/issues/293)
- Cannot set profile picture [\#273](https://github.com/uTox/uTox/issues/273)
- uTox GUI freeze [\#259](https://github.com/uTox/uTox/issues/259)
- Messages.c Refactor [\#128](https://github.com/uTox/uTox/issues/128)

**Closed issues:**

- 0.8.0 Initiating file transfer crashes utox [\#303](https://github.com/uTox/uTox/issues/303)
- uTox crashes when attempting to send a file [\#302](https://github.com/uTox/uTox/issues/302)
- about double release post [\#292](https://github.com/uTox/uTox/issues/292)
- Defunct video input device selection [\#198](https://github.com/uTox/uTox/issues/198)
- guide fo utox compilation on Tails [\#80](https://github.com/uTox/uTox/issues/80)

**Merged pull requests:**

- fix text color in selected group icon [\#306](https://github.com/uTox/uTox/pull/306) ([cebe](https://github.com/cebe))
- added user icons for mini roster [\#304](https://github.com/uTox/uTox/pull/304) ([cebe](https://github.com/cebe))
- re-enable proxy support [\#301](https://github.com/uTox/uTox/pull/301) ([GrayHatter](https://github.com/GrayHatter))
- Mini friends list  [\#299](https://github.com/uTox/uTox/pull/299) ([GrayHatter](https://github.com/GrayHatter))
- Group icon [\#296](https://github.com/uTox/uTox/pull/296) ([GrayHatter](https://github.com/GrayHatter))
- copy log functions to cocoa [\#294](https://github.com/uTox/uTox/pull/294) ([stal888](https://github.com/stal888))

## [v0.8.0](https://github.com/uTox/uTox/tree/v0.8.0) (2016-04-20)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.7.0...v0.8.0)

**Implemented enhancements:**

- \[Feature request\] Font size settings [\#114](https://github.com/uTox/uTox/issues/114)

**Fixed bugs:**

- Preferences on Mac does nothing [\#287](https://github.com/uTox/uTox/issues/287)
- uTox 0.7.0 OSX - cannot start [\#258](https://github.com/uTox/uTox/issues/258)
- Tiny interface in android [\#256](https://github.com/uTox/uTox/issues/256)
- Old ID disappears [\#252](https://github.com/uTox/uTox/issues/252)
- profile seg fault after update [\#197](https://github.com/uTox/uTox/issues/197)
- Segfault [\#158](https://github.com/uTox/uTox/issues/158)

**Closed issues:**

- OSX: resizing window too small [\#289](https://github.com/uTox/uTox/issues/289)
- tox: URLs not Getting Parsed [\#286](https://github.com/uTox/uTox/issues/286)
- Windows 10 system setting profile password crash [\#280](https://github.com/uTox/uTox/issues/280)
- Update utox.org statement. [\#278](https://github.com/uTox/uTox/issues/278)
- Release version 0.7.0 for linux not available: The specified key does not exist. [\#277](https://github.com/uTox/uTox/issues/277)
- A new Chinese Translate [\#261](https://github.com/uTox/uTox/issues/261)
- uTox is raiding a SBIE2314 error if started in Sandboxie Sandbox [\#253](https://github.com/uTox/uTox/issues/253)
- dual webcam broken [\#200](https://github.com/uTox/uTox/issues/200)
- DPI setting ignored at start [\#199](https://github.com/uTox/uTox/issues/199)
- re-add alt+num on windows [\#192](https://github.com/uTox/uTox/issues/192)
- Client Crashing Resets all Settings to Default on next Start \(WIN-10\) [\#175](https://github.com/uTox/uTox/issues/175)
- uTox package in Debian Jessie repo - gone [\#161](https://github.com/uTox/uTox/issues/161)

**Merged pull requests:**

- cocoa: add tox: url handling and limit window size [\#290](https://github.com/uTox/uTox/pull/290) ([stal888](https://github.com/stal888))
- fix SYSROOT default setting [\#288](https://github.com/uTox/uTox/pull/288) ([felix-salfelder](https://github.com/felix-salfelder))
- Prefix cocoa folder with src/ [\#285](https://github.com/uTox/uTox/pull/285) ([GrayHatter](https://github.com/GrayHatter))
- android build [\#283](https://github.com/uTox/uTox/pull/283) ([felix-salfelder](https://github.com/felix-salfelder))
- attempting to make tools/android-build.sh work \(WIP\) [\#279](https://github.com/uTox/uTox/pull/279) ([felix-salfelder](https://github.com/felix-salfelder))
- fix some bugs [\#274](https://github.com/uTox/uTox/pull/274) ([stal888](https://github.com/stal888))
- Update utox.desktop [\#266](https://github.com/uTox/uTox/pull/266) ([cebe](https://github.com/cebe))
- Update tw.h [\#264](https://github.com/uTox/uTox/pull/264) ([lineteen](https://github.com/lineteen))
- Update cn.h [\#263](https://github.com/uTox/uTox/pull/263) ([lineteen](https://github.com/lineteen))
- Update German translations [\#260](https://github.com/uTox/uTox/pull/260) ([sfan5](https://github.com/sfan5))
- Added Russian transation for the pop-up comment of the shortcut [\#257](https://github.com/uTox/uTox/pull/257) ([TotalCaesar659](https://github.com/TotalCaesar659))
- consistent usage of checkinstall [\#249](https://github.com/uTox/uTox/pull/249) ([stemd](https://github.com/stemd))
- Fix application crash with variable length frame size webcams. [\#152](https://github.com/uTox/uTox/pull/152) ([abbat](https://github.com/abbat))

## [v0.7.0](https://github.com/uTox/uTox/tree/v0.7.0) (2016-03-21)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.6.1...v0.7.0)

**Fixed bugs:**

- Bottom elements on the friend panel are inactive while panel does  not fit in the window [\#242](https://github.com/uTox/uTox/issues/242)
- Scrolling with the mouse wheel is very slow [\#237](https://github.com/uTox/uTox/issues/237)
- Error making call xlib \(utox Alpha version 0.6.1\) [\#230](https://github.com/uTox/uTox/issues/230)
- Profile / ID disappearing [\#188](https://github.com/uTox/uTox/issues/188)
- AltGr+number must not switch tab [\#157](https://github.com/uTox/uTox/issues/157)
- preview window does not close if opening video device failed [\#56](https://github.com/uTox/uTox/issues/56)
- Some UI items are too wide [\#3](https://github.com/uTox/uTox/issues/3)

**Closed issues:**

- No longer able to set avatar image [\#245](https://github.com/uTox/uTox/issues/245)
- Feedback from another cross-platform test... [\#244](https://github.com/uTox/uTox/issues/244)
- Calls have no audio, Call buttons not functioning [\#243](https://github.com/uTox/uTox/issues/243)
- 1-pixel wide tray icon [\#235](https://github.com/uTox/uTox/issues/235)
- error: ‘FILE\_TRANSFER’ has no member named ‘filenumber’ [\#234](https://github.com/uTox/uTox/issues/234)
- utox.org v. tox.chat [\#229](https://github.com/uTox/uTox/issues/229)
- Cross-compile script for Windows is outdated. [\#226](https://github.com/uTox/uTox/issues/226)
- Can not cancel any call [\#222](https://github.com/uTox/uTox/issues/222)
- Switching video input from desktop to "none" crashes [\#210](https://github.com/uTox/uTox/issues/210)
- Some issues [\#186](https://github.com/uTox/uTox/issues/186)
- WebRTC built into uTOX [\#176](https://github.com/uTox/uTox/issues/176)

**Merged pull requests:**

- Reduce warnings [\#248](https://github.com/uTox/uTox/pull/248) ([benwaffle](https://github.com/benwaffle))
- Make installation process more tunable \(adjust pkg-config name and datarootdir\) [\#247](https://github.com/uTox/uTox/pull/247) ([ony](https://github.com/ony))
- Fix for issue \#237 mouse wheel scroll is very slow [\#241](https://github.com/uTox/uTox/pull/241) ([master-passeli](https://github.com/master-passeli))
- Fix for issue 230, Error making call [\#240](https://github.com/uTox/uTox/pull/240) ([master-passeli](https://github.com/master-passeli))
- updated Ukrainian translation [\#228](https://github.com/uTox/uTox/pull/228) ([yalvex](https://github.com/yalvex))
- Update of Polish translation [\#225](https://github.com/uTox/uTox/pull/225) ([dagashiya](https://github.com/dagashiya))
- Fixes file save dialog title [\#220](https://github.com/uTox/uTox/pull/220) ([cebe](https://github.com/cebe))

## [v0.6.1](https://github.com/uTox/uTox/tree/v0.6.1) (2016-02-29)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.6.0...v0.6.1)

**Fixed bugs:**

- Password "show" button has wrong tooltip [\#209](https://github.com/uTox/uTox/issues/209)
- Tooltip for the settings button is "Add New Contact" on start [\#90](https://github.com/uTox/uTox/issues/90)

**Closed issues:**

- Problem to close the preview camera [\#218](https://github.com/uTox/uTox/issues/218)
- Unable to select avatar in linux client [\#215](https://github.com/uTox/uTox/issues/215)
- Can't accept calls: uTox:	Error trying to toxav\_answer error \(1\) [\#208](https://github.com/uTox/uTox/issues/208)
- \[HELP NEEDED\] README.md is out of date. [\#37](https://github.com/uTox/uTox/issues/37)
- Adding friends/switching to settings requires too many clicks [\#20](https://github.com/uTox/uTox/issues/20)

**Merged pull requests:**

- GTK: fix avatar chooser [\#216](https://github.com/uTox/uTox/pull/216) ([benwaffle](https://github.com/benwaffle))
- Fix dmg script [\#214](https://github.com/uTox/uTox/pull/214) ([RubenRocha](https://github.com/RubenRocha))

## [v0.6.0](https://github.com/uTox/uTox/tree/v0.6.0) (2016-02-21)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.5.1...v0.6.0)

**Implemented enhancements:**

- use getopt to parse args [\#142](https://github.com/uTox/uTox/issues/142)
- uTox password security aspects [\#130](https://github.com/uTox/uTox/issues/130)
- uTox doesn't support jpg avatars [\#1](https://github.com/uTox/uTox/issues/1)

**Fixed bugs:**

- No ringtone? [\#189](https://github.com/uTox/uTox/issues/189)
- TOX ID disappears in windows [\#187](https://github.com/uTox/uTox/issues/187)
- uTox password security aspects [\#130](https://github.com/uTox/uTox/issues/130)
- uTox starts with extremely low DPI and increases it with each new startup [\#124](https://github.com/uTox/uTox/issues/124)
- Tooltip in system tray is always colon [\#115](https://github.com/uTox/uTox/issues/115)

**Closed issues:**

- sending a screen capture from utox to utox does not ask for acceptance [\#174](https://github.com/uTox/uTox/issues/174)
- Closing client should minimize to taskbar to avoid clutter [\#168](https://github.com/uTox/uTox/issues/168)
- How to compile uTox in VS [\#151](https://github.com/uTox/uTox/issues/151)
- UTox win64-0.5.0 [\#149](https://github.com/uTox/uTox/issues/149)
- Old "Normal" DPI is at 90% [\#125](https://github.com/uTox/uTox/issues/125)
- 100% cpu load on video call. [\#67](https://github.com/uTox/uTox/issues/67)

**Merged pull requests:**

- Av refactor \[DO NOT MERGE\] [\#196](https://github.com/uTox/uTox/pull/196) ([GrayHatter](https://github.com/GrayHatter))
- fixed \#187 [\#190](https://github.com/uTox/uTox/pull/190) ([ingvar1995](https://github.com/ingvar1995))
- Add an error check to /sendfile command [\#183](https://github.com/uTox/uTox/pull/183) ([elgis](https://github.com/elgis))
- fixes \#125 [\#177](https://github.com/uTox/uTox/pull/177) ([danielisaksen](https://github.com/danielisaksen))
- Fix GTK error [\#170](https://github.com/uTox/uTox/pull/170) ([benwaffle](https://github.com/benwaffle))
- Fixed some bugs [\#166](https://github.com/uTox/uTox/pull/166) ([ingvar1995](https://github.com/ingvar1995))
- Support jpg avatars [\#165](https://github.com/uTox/uTox/pull/165) ([benwaffle](https://github.com/benwaffle))
- Fixed \#151 [\#155](https://github.com/uTox/uTox/pull/155) ([linuxraspilxc](https://github.com/linuxraspilxc))

## [v0.5.1](https://github.com/uTox/uTox/tree/v0.5.1) (2016-01-05)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.5.0...v0.5.1)

**Implemented enhancements:**

- Add Esperanto translation [\#113](https://github.com/uTox/uTox/issues/113)
- GTK file chooser doesn't preview images [\#93](https://github.com/uTox/uTox/issues/93)
- Video quality / compression [\#23](https://github.com/uTox/uTox/issues/23)

**Fixed bugs:**

- Makefile should build changed files [\#110](https://github.com/uTox/uTox/issues/110)
- uTox crash on start [\#109](https://github.com/uTox/uTox/issues/109)
- OS X: Unicode status message or display name causes crash on launch [\#102](https://github.com/uTox/uTox/issues/102)
- white line instead of system tray icon in MATE [\#16](https://github.com/uTox/uTox/issues/16)

**Closed issues:**

- Adding my best friend failed [\#147](https://github.com/uTox/uTox/issues/147)
- DPI says BUG PLEASE REPORT [\#138](https://github.com/uTox/uTox/issues/138)
- Android issues [\#131](https://github.com/uTox/uTox/issues/131)

**Merged pull requests:**

- skel for windows travis support [\#148](https://github.com/uTox/uTox/pull/148) ([GrayHatter](https://github.com/GrayHatter))
- Some users don't use encryption [\#141](https://github.com/uTox/uTox/pull/141) ([benwaffle](https://github.com/benwaffle))
- Make lang & str counts a bit simpler [\#140](https://github.com/uTox/uTox/pull/140) ([benwaffle](https://github.com/benwaffle))
- Implement some per-friend settings [\#137](https://github.com/uTox/uTox/pull/137) ([tsudoko](https://github.com/tsudoko))
- Fix compilation on arm [\#123](https://github.com/uTox/uTox/pull/123) ([doughdemon](https://github.com/doughdemon))
- Better fix for the font crash [\#122](https://github.com/uTox/uTox/pull/122) ([benwaffle](https://github.com/benwaffle))
- Fix crash with fonts [\#121](https://github.com/uTox/uTox/pull/121) ([benwaffle](https://github.com/benwaffle))
- Gtk file chooser preview [\#120](https://github.com/uTox/uTox/pull/120) ([benwaffle](https://github.com/benwaffle))
- Linux makefile [\#119](https://github.com/uTox/uTox/pull/119) ([benwaffle](https://github.com/benwaffle))
- Remove \_\_APPLE\_\_ checks in xlib/ [\#118](https://github.com/uTox/uTox/pull/118) ([benwaffle](https://github.com/benwaffle))
- Add Esperanto translation from \#113 [\#116](https://github.com/uTox/uTox/pull/116) ([tsudoko](https://github.com/tsudoko))
- Added full build instructions for ubuntu [\#112](https://github.com/uTox/uTox/pull/112) ([linuxraspilxc](https://github.com/linuxraspilxc))
- fixed "Online contacts" being called "All" [\#106](https://github.com/uTox/uTox/pull/106) ([Doom032](https://github.com/Doom032))
- Travis src fix [\#104](https://github.com/uTox/uTox/pull/104) ([GrayHatter](https://github.com/GrayHatter))
- Update BUILD.md to refer to COCOA.md [\#103](https://github.com/uTox/uTox/pull/103) ([cybercatgurrl](https://github.com/cybercatgurrl))
- Master [\#101](https://github.com/uTox/uTox/pull/101) ([GrayHatter](https://github.com/GrayHatter))

## [v0.5.0](https://github.com/uTox/uTox/tree/v0.5.0) (2015-12-20)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.4.6...v0.5.0)

## [v0.4.6](https://github.com/uTox/uTox/tree/v0.4.6) (2015-12-20)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.4.5...v0.4.6)

**Implemented enhancements:**

- File transfer layout glitch [\#57](https://github.com/uTox/uTox/issues/57)

**Fixed bugs:**

- Friends list context menu behaves weird after right-clicking the left side of the friends list [\#99](https://github.com/uTox/uTox/issues/99)
- There's no way to remove the avatar [\#98](https://github.com/uTox/uTox/issues/98)
- Groupchat context menu items are broken [\#92](https://github.com/uTox/uTox/issues/92)
- There's no way to create a new groupchat [\#91](https://github.com/uTox/uTox/issues/91)
- OS X: Typing notification too low [\#87](https://github.com/uTox/uTox/issues/87)
- OS X: uTox crashes when typing [\#86](https://github.com/uTox/uTox/issues/86)
- File transfer layout glitch [\#57](https://github.com/uTox/uTox/issues/57)
- µTox crashes when trying to use proxy with bad proxy settings [\#54](https://github.com/uTox/uTox/issues/54)
- Typing notification is covered by input field area. [\#26](https://github.com/uTox/uTox/issues/26)
- Friends list header is 1px wider than the rest of the friends list [\#17](https://github.com/uTox/uTox/issues/17)

**Closed issues:**

- Make file doesn't work on Windows [\#73](https://github.com/uTox/uTox/issues/73)

**Merged pull requests:**

- fix minor memory leak [\#89](https://github.com/uTox/uTox/pull/89) ([stal888](https://github.com/stal888))
- GTK: switch to GTK3, use constants, and organize code [\#88](https://github.com/uTox/uTox/pull/88) ([benwaffle](https://github.com/benwaffle))
- Add encrypt save file support. [\#85](https://github.com/uTox/uTox/pull/85) ([GrayHatter](https://github.com/GrayHatter))

## [v0.4.5](https://github.com/uTox/uTox/tree/v0.4.5) (2015-12-09)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.4.4develop...v0.4.5)

**Fixed bugs:**

- Can not add friend when settings are openend [\#83](https://github.com/uTox/uTox/issues/83)

**Merged pull requests:**

- adjust makefile to recompile on header change [\#77](https://github.com/uTox/uTox/pull/77) ([cebe](https://github.com/cebe))
- New Interface changes because stal hate's the current. [\#43](https://github.com/uTox/uTox/pull/43) ([GrayHatter](https://github.com/GrayHatter))

## [v0.4.4develop](https://github.com/uTox/uTox/tree/v0.4.4develop) (2015-12-08)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.4.4...v0.4.4develop)

**Implemented enhancements:**

- Keyboard shortcuts to switch between friends [\#51](https://github.com/uTox/uTox/issues/51)
- Contact search box doesn't search contact aliases. [\#5](https://github.com/uTox/uTox/issues/5)

**Fixed bugs:**

- bug combining greentext and redtext [\#72](https://github.com/uTox/uTox/issues/72)
- Broken custom theme support. [\#28](https://github.com/uTox/uTox/issues/28)

**Closed issues:**

- Feature: OS X video window resizing [\#74](https://github.com/uTox/uTox/issues/74)
- \[HELP NEEDED\] fix .travis.yaml to check build of uTox for PRs [\#68](https://github.com/uTox/uTox/issues/68)
- Scale cursor with DPI [\#18](https://github.com/uTox/uTox/issues/18)

**Merged pull requests:**

- New file transfer ui [\#81](https://github.com/uTox/uTox/pull/81) ([stal888](https://github.com/stal888))
- cocoa: make video window resizable [\#76](https://github.com/uTox/uTox/pull/76) ([stal888](https://github.com/stal888))
- use blobs from build.tox.chat [\#71](https://github.com/uTox/uTox/pull/71) ([stal888](https://github.com/stal888))
- fix travis [\#70](https://github.com/uTox/uTox/pull/70) ([stal888](https://github.com/stal888))
- Update cn.h [\#69](https://github.com/uTox/uTox/pull/69) ([giwhub](https://github.com/giwhub))
- updated manpage and added --help option to xlib main.c [\#66](https://github.com/uTox/uTox/pull/66) ([cebe](https://github.com/cebe))
- Give the filter button more vertical space [\#63](https://github.com/uTox/uTox/pull/63) ([tsudoko](https://github.com/tsudoko))
- Add missing COLOR\_REDTEXT entry to the color table [\#62](https://github.com/uTox/uTox/pull/62) ([tsudoko](https://github.com/tsudoko))
- Move text from the search field to the ID field after clicking "add" instead of syncing it between both fields [\#61](https://github.com/uTox/uTox/pull/61) ([tsudoko](https://github.com/tsudoko))
- remove SelectionRequest debug message [\#60](https://github.com/uTox/uTox/pull/60) ([Doom032](https://github.com/Doom032))
- let search box search aliases as well [\#59](https://github.com/uTox/uTox/pull/59) ([Doom032](https://github.com/Doom032))
- do not block video device when video is not used [\#52](https://github.com/uTox/uTox/pull/52) ([cebe](https://github.com/cebe))
- refactor list searching & filtering, and add previous/next tab shortcuts on Linux and Windows [\#47](https://github.com/uTox/uTox/pull/47) ([Doom032](https://github.com/Doom032))
- Enable auto changing bitrate for uTox video [\#42](https://github.com/uTox/uTox/pull/42) ([GrayHatter](https://github.com/GrayHatter))
- Add bg and icon to linux tray icon [\#14](https://github.com/uTox/uTox/pull/14) ([GrayHatter](https://github.com/GrayHatter))

## [v0.4.4](https://github.com/uTox/uTox/tree/v0.4.4) (2015-11-19)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.4.2...v0.4.4)

**Fixed bugs:**

- Initial window paint relies on racy window manager resizing to work properly [\#40](https://github.com/uTox/uTox/issues/40)
- Avatar is not being sent when there's an ongoing file transfer to contact [\#39](https://github.com/uTox/uTox/issues/39)
- TRUE is not defined in xlib/main.c and others [\#38](https://github.com/uTox/uTox/issues/38)
- Ctrl+A not working if caps lock is on [\#35](https://github.com/uTox/uTox/issues/35)

**Merged pull requests:**

- Add & fix some tooltips, slight cleanup of UI strings [\#53](https://github.com/uTox/uTox/pull/53) ([Doom032](https://github.com/Doom032))
- Issue 35 [\#50](https://github.com/uTox/uTox/pull/50) ([GrayHatter](https://github.com/GrayHatter))
- text changes [\#49](https://github.com/uTox/uTox/pull/49) ([stal888](https://github.com/stal888))
- fixed LC\_ALL not being looked at on xlib [\#48](https://github.com/uTox/uTox/pull/48) ([Doom032](https://github.com/Doom032))
- load widow sizes from save in xlib [\#41](https://github.com/uTox/uTox/pull/41) ([GrayHatter](https://github.com/GrayHatter))
- Fix Build Errors and Warnings. [\#36](https://github.com/uTox/uTox/pull/36) ([lluixhi](https://github.com/lluixhi))
- fixed not losing focus of edit\_msg when clicking the messages panel [\#33](https://github.com/uTox/uTox/pull/33) ([Doom032](https://github.com/Doom032))

## [v0.4.2](https://github.com/uTox/uTox/tree/v0.4.2) (2015-11-03)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.3.2...v0.4.2)

**Merged pull requests:**

- Develop into master [\#25](https://github.com/uTox/uTox/pull/25) ([GrayHatter](https://github.com/GrayHatter))
- fix osx bugs [\#24](https://github.com/uTox/uTox/pull/24) ([stal888](https://github.com/stal888))
- Allow adding LDFLAGS from env variable [\#21](https://github.com/uTox/uTox/pull/21) ([benwaffle](https://github.com/benwaffle))
- ToxAV new api. [\#15](https://github.com/uTox/uTox/pull/15) ([GrayHatter](https://github.com/GrayHatter))
- build fixes for OS X 10.9 [\#11](https://github.com/uTox/uTox/pull/11) ([soyersoyer](https://github.com/soyersoyer))
- plug memory leak [\#10](https://github.com/uTox/uTox/pull/10) ([benwaffle](https://github.com/benwaffle))
- tweaks to de translation [\#9](https://github.com/uTox/uTox/pull/9) ([aaannndddyyy](https://github.com/aaannndddyyy))
- tweaks to Spanish translation [\#7](https://github.com/uTox/uTox/pull/7) ([aaannndddyyy](https://github.com/aaannndddyyy))
- Get rid of empty space left after the transfer button [\#4](https://github.com/uTox/uTox/pull/4) ([tsudoko](https://github.com/tsudoko))
- Reduce spacing between bars in the hamburger menu button [\#2](https://github.com/uTox/uTox/pull/2) ([tsudoko](https://github.com/tsudoko))

## [v0.3.2](https://github.com/uTox/uTox/tree/v0.3.2) (2015-06-07)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.3.1...v0.3.2)

## [v0.3.1](https://github.com/uTox/uTox/tree/v0.3.1) (2015-05-31)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.3...v0.3.1)

## [v0.3](https://github.com/uTox/uTox/tree/v0.3) (2015-05-18)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.s...v0.3)

## [v0.2.s](https://github.com/uTox/uTox/tree/v0.2.s) (2015-05-14)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.r...v0.2.s)

## [v0.2.r](https://github.com/uTox/uTox/tree/v0.2.r) (2015-05-08)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.q...v0.2.r)

## [v0.2.q](https://github.com/uTox/uTox/tree/v0.2.q) (2015-05-01)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.o...v0.2.q)

## [v0.2.o](https://github.com/uTox/uTox/tree/v0.2.o) (2015-03-11)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.n...v0.2.o)

## [v0.2.n](https://github.com/uTox/uTox/tree/v0.2.n) (2015-02-12)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.m...v0.2.n)

## [v0.2.m](https://github.com/uTox/uTox/tree/v0.2.m) (2015-02-05)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.l...v0.2.m)

## [v0.2.l](https://github.com/uTox/uTox/tree/v0.2.l) (2015-01-30)
[Full Changelog](https://github.com/uTox/uTox/compare/v0.2.k...v0.2.l)

## [v0.2.k](https://github.com/uTox/uTox/tree/v0.2.k) (2015-01-28)


\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*
