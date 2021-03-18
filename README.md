# abi-stable-v8

## Documentation

Reference on ABI compatibility: https://community.kde.org/Policies/Binary_Compatibility_Issues_With_C%2B%2B

Additional commits made for ABI-stability are prefixed with `[ABI-stability]`.

### Local setup

- Clone this repository
- Add the V8 remote: `git remote add v8 https://chromium.googlesource.com/v8/v8.git`

### List commits to audit

```bash
git fetch v8 master
git log HEAD..v8/master --stat --reverse -- 'include/cppgc/common.h' 'include/v8.h' 'include/v8-internal.h' 'include/v8-platform.h' 'include/v8-profiler.h' 'include/v8-version.h' 'include/v8config.h'
```

## V8 9.0

Branch: https://github.com/targos/abi-stable-v8/commits/9.0-abi-stable

Reference commit: https://github.com/v8/v8/commit/349bcc6a075411f1a7ce2d866c3dfeefc2efa39d

### Changes

- [15a3932250](https://github.com/targos/abi-stable-v8/commit/15a39322507b9bb8c0f6aabc5814c2d87ede2f62) - Reverted [[api] Avoid handles for const API functions](https://github.com/v8/v8/commit/aee471b2ff5b1a9e622426454885b748d226535b)
- [d6bc672189](https://github.com/targos/abi-stable-v8/commit/d6bc67218954f2e7d691316f9518ae598bdb6483) - Reverted [[api] Remove deprecated [Shared]ArrayBuffer API](https://github.com/v8/v8/commit/578f6be77fc5d8af975005c2baf918e7225abb62)
