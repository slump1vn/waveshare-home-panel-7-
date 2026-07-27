# Security

Do not publish Wi-Fi passwords, Home Assistant access tokens, private camera
URLs or the contents of `secrets.yaml`.

If credentials are accidentally committed:

1. Revoke or change the exposed credential immediately.
2. Remove it from the repository history.
3. Create a new credential.
4. Confirm that `secrets.yaml` remains listed in `.gitignore`.
