# abi-stable-v8

## Documentation

Additional commits made for ABI-stability are prefixed with `[ABI-stability]`.

### List commits to audit

```bash
git fetch v8 master
git log HEAD..v8/master --reverse -- include
```

## V8 9.0

Branch: https://github.com/targos/abi-stable-v8/commits/9.0-abi-stable

Reference commit: https://github.com/v8/v8/commit/349bcc6a075411f1a7ce2d866c3dfeefc2efa39d

### Changes

- [15a3932250](https://github.com/targos/abi-stable-v8/commit/15a39322507b9bb8c0f6aabc5814c2d87ede2f62) - Reverted [[api] Avoid handles for const API functions](https://github.com/v8/v8/commit/aee471b2ff5b1a9e622426454885b748d226535b)
