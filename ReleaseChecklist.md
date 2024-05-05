# wxMaxima Release Checklist

## GitHub documentation about managing releases:

<https://docs.github.com/en/github/administering-a-repository/managing-releases-in-a-repository>

## wxMaxima (additional) steps:

- Does the current git version compile? Do the checks on GitHub work?
- Enter the new version number into CMakeLists.txt
- Update the version numbers in the 'docker-wxmaxima' repository (update_versions.sh)
- Update NEWS.md in order to announce the new version on <https://freshcode.club/>
- Update data/io.github.wxmaxima_developers.wxMaxima.appdata.xml with the information
  about the new release. Most html tags are forbidden by flatpack or appImage
  builders.
- Validate the appdata file with:
  appstream-util validate data/io.github.wxmaxima_developers.wxMaxima.appdata.xml
- Update snap/snapcraft.yaml
- Run "make test"
- Does test/testbench_simple.wxmx work?
- Update the included HTML manuals.
- Create an (annotated: using "git tag -a") git tag for the release
- Push the tag to GitHub, using: git push origin --tags
- Go to the releases page GitHub and convert the tag into a release.
  If possible add an Windows installer too.
  Be sure, to remove the 'draft' status. Log out from GitHub and check,
  if you see the release as an anonymous user too.
- Update the release info in the files download.html and in version.txt
  in the gh_pages branch.
- Set VERSION_SUFFIX to "DevelopmentSnapshot" in CMakeLists.txt for the new development version.
- Download the tarball (.tar.gz and .zip version) and run the following command on them:
  gpg --armor --detach-sign <filename>
- On the release page on github modify the release to contain the two .asc files
  the command produced.
- In Maxima's source tree in crosscompile-windows/wxmaxima/CMakeLists.txt: Change the
  version number and the MD5 sum of the release tarball to the newest value.
- Create a Windows installer using the 'Crosscompiled-Windows-installer' repository
  and add it to the release.
