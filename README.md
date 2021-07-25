# abi-stable-v8

## Tracked V8 versions

- [V8 9.0 - Node.js 16.x](./versions/9.0.md)

## Documentation

Reference on ABI compatibility: <https://community.kde.org/Policies/Binary_Compatibility_Issues_With_C%2B%2B>

Additional commits made for ABI-stability are prefixed with `[ABI-stability]`.

### Local setup

- Clone this repository
- Add the V8 remote: `git remote add v8 https://chromium.googlesource.com/v8/v8.git`

### List commits to audit

```bash
git fetch v8 master
git log HEAD..v8/master --stat --reverse -- 'include/cppgc/common.h' 'include/libplatform/libplatform.h'  'include/libplatform/libplatform-export.h' 'include/libplatform/v8-tracing.h' 'include/v8.h' 'include/v8-internal.h' 'include/v8-platform.h' 'include/v8-profiler.h' 'include/v8-version.h' 'include/v8config.h'
```
