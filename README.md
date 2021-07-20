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
git log HEAD..v8/master --stat --reverse -- 'include/cppgc/common.h' 'include/libplatform/libplatform.h'  'include/libplatform/libplatform-export.h' 'include/libplatform/v8-tracing.h' 'include/v8.h' 'include/v8-internal.h' 'include/v8-platform.h' 'include/v8-profiler.h' 'include/v8-version.h' 'include/v8config.h'
```

## V8 9.0

Branch: https://github.com/targos/abi-stable-v8/commits/9.0-abi-stable

Branch point of 9.0: https://github.com/v8/v8/commit/349bcc6a075411f1a7ce2d866c3dfeefc2efa39d

### Changes

#### V8 9.0 to 9.1

Branch point of 9.1: https://github.com/v8/v8/commit/f565e72d5ba88daae35a59d0f978643e2343e912

ABI-stable branch: https://github.com/targos/abi-stable-v8/tree/9.0-abi-stable-9.1

Diff: https://github.com/targos/abi-stable-v8/compare/f565e72d5ba88daae35a59d0f978643e2343e912...9.0-abi-stable-9.1

- [15a3932250](https://github.com/targos/abi-stable-v8/commit/15a39322507b9bb8c0f6aabc5814c2d87ede2f62) - Reverted [[api] Avoid handles for const API functions](https://github.com/v8/v8/commit/aee471b2ff5b1a9e622426454885b748d226535b)
- [d6bc672189](https://github.com/targos/abi-stable-v8/commit/d6bc67218954f2e7d691316f9518ae598bdb6483) - Reverted [[api] Remove deprecated [Shared]ArrayBuffer API](https://github.com/v8/v8/commit/578f6be77fc5d8af975005c2baf918e7225abb62)
- [eb7f50e6ee](https://github.com/targos/abi-stable-v8/commit/eb7f50e6eebbfebfa7997f45b480b093bf8d46bc) - Reverted [[Jobs]: Cleanup in v8 platform.](https://github.com/v8/v8/commit/baf2b088dd9f585aa597459f30d71431171666e2)
- [8565c87209](https://github.com/targos/abi-stable-v8/commit/8565c87209ba74a2bf0190f00ddc76b63079a93e) - Reverted [Skip global registration of [Shared]ArrayBuffer backing stores](https://github.com/v8/v8/commit/fcdf35e6d70d51699ece063e25dc705e80673308)

#### V8 9.1 to 9.2

Branch point of 9.2: https://github.com/v8/v8/commit/587a04f02ab0487d194b55a7137dc2045e071597

ABI-stable branch: https://github.com/targos/abi-stable-v8/tree/9.0-abi-stable-9.2

Diff: https://github.com/targos/abi-stable-v8/compare/587a04f02ab0487d194b55a7137dc2045e071597...9.0-abi-stable-9.2

- [ed5b25f698](https://github.com/targos/abi-stable-v8/commit/ed5b25f6986a32df20fe0fb99ab79e11dd7f82e1) - Reverted [[api] Remove previously deprecated Function::GetDisplayName().](https://github.com/v8/v8/commit/6165fef8cc9dde52973e54c915e6905221b3f8fb)
- [ed7f09bf18](https://github.com/targos/abi-stable-v8/commit/ed7f09bf181d596376cdc84c724547e0e485c5a2) - Reverted [[api] Remove deprecated Symbol::Name()](https://github.com/v8/v8/commit/bbc72ef6c7d6d8e2c4dd074d7713e5c841003163)
- [a39efa8cd6](https://github.com/targos/abi-stable-v8/commit/a39efa8cd6bd5b864e56ad76c5dc89231d5e9f30) - Reverted API change from [Heap Number encoding](https://github.com/v8/v8/commit/7f52e4f92d3d3ded9a1701ee2f93966075ae5004)
- [a5913d18a2](https://github.com/targos/abi-stable-v8/commit/a5913d18a29bf04101a11d9afd3fc4d650196669) - Reverted [[cpu-profiler] Reintroduce support for context filtering](https://github.com/v8/v8/commit/0aacfb2a6ecbeda1d1d97ca113afd8253a1b9670)
- [c01a294cb8](https://github.com/targos/abi-stable-v8/commit/c01a294cb85fc457901615056f0a3fca60a20d8e) - Restored changed signature from [Heap Number encoding](https://github.com/v8/v8/commit/7f52e4f92d3d3ded9a1701ee2f93966075ae5004) under a new method name
- [93d473308d](https://github.com/targos/abi-stable-v8/commit/0503055d59c5aabe38a0fa4a719583faf2b7d1d2) - Remove deprecation because of self-triggered warning

#### V8 9.2 to 9.3

Branch point of 9.3: https://github.com/v8/v8/commit/4b6b4cabf3b6a20cdfda72b369df49f3311c4344

ABI-stable branch: https://github.com/targos/abi-stable-v8/tree/9.0-abi-stable-9.3

Diff: https://github.com/targos/abi-stable-v8/compare/4b6b4cabf3b6a20cdfda72b369df49f3311c4344...9.0-abi-stable-9.3

- [38690c8058](https://github.com/targos/abi-stable-v8/commit/38690c80588466d71c5e6d6cbb758808c9eb2123) - Reverted [[api] Support PropertyAttribute in v8::Template::Set](https://github.com/v8/v8/commit/22a32f11f7a39e4a73105608a51ebab0ad97960f)
- [497ca12865](https://github.com/targos/abi-stable-v8/commit/497ca12865434cf30a7dc52de7dfbb67d1aeb649) - Reverted API change from [[API] Allow embedders to assign instance types](https://github.com/v8/v8/commit/5f82dbbe534635a8b1e93f27e8fd7e68c0eaf291)

#### V8 9.3 to 9.4

Branch point of 9.4: TBD

ABI-stable branch: TBD

Diff: TBD

No changes for now.
