# Uploading this project to GitHub

## Browser method

1. Sign in to GitHub.
2. Select the **+** menu and choose **New repository**.
3. Use the repository name `waveshare-home-panel`.
4. Choose **Private** unless you are certain the configuration contains no
   private entity names, addresses or tokens.
5. Do not initialise the repository with a README, licence or `.gitignore`,
   because this project already includes them.
6. Select **Create repository**.
7. On the empty repository page, select **uploading an existing file**.
8. Drag every file and folder from the extracted project into the upload area.
9. Enter the commit message `Initial release v120`.
10. Select **Commit changes**.

GitHub's browser uploader may not preserve empty folders, which is harmless.

## GitHub Desktop method

1. Install GitHub Desktop.
2. Extract this project.
3. In GitHub Desktop, select **File > Add local repository**.
4. Select the extracted `waveshare-home-panel-github` folder.
5. If prompted, choose **create a repository here**.
6. Commit the files with the message `Initial release v120`.
7. Select **Publish repository**.
8. Choose the repository name and visibility, then publish.

## Command-line method

Create an empty repository on GitHub, then run:

```powershell
cd "C:\path\to\waveshare-home-panel-github"
git init
git add .
git commit -m "Initial release v120"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/waveshare-home-panel.git
git push -u origin main
```

Replace `YOUR-USERNAME` with your GitHub username.
