# OSY – Git Helper (krátka verzia)

## Základné nastavenie (raz na stroji)
```
git config --global user.name "Tvoje Meno"
git config --global user.email "tvoj@mail"
git config --global init.defaultBranch main
git config --global pull.rebase false      # alebo true (ak preferuješ rebase)
git config --global core.editor "code --wait"
```

## Nový / existujúci projekt
```
git init                 # inicializuj repozitár v aktuálnom priečinku
git clone <url>          # naklonuj repozitár
```

## Stav, pridanie, commit
```
git status
git add <súbor> | .
git commit -m "správa"
git restore --staged <súbor>
git restore <súbor>
git diff                 # pracovné zmeny
git diff --staged        # staged diff
git log --oneline --graph --decorate
```

## Vetvy (branching)
```
git branch
git branch <nová-vetva>
git switch <vetva>         # alebo: git checkout <vetva>
git switch -c <nová-vetva>
git merge <vetva>
git rebase <vetva>
```

## Remote (push/pull/fetch)
```
git remote -v
git remote add origin <url>
git push -u origin <vetva>
git push
git pull
git fetch
```

## Stash (rozpracované veci)
```
git stash
git stash list
git stash pop
git stash apply
```

## Tagy (releasy)
```
git tag v1.0
git tag -a v1.0 -m "release 1.0"
git push origin v1.0
git push --tags
```

## Riešenie konfliktov – postup
1. `git pull` / `merge` / `rebase` → konflikt
2. Otvor konfliktné súbory vo VS Code (`<<<<<<<`, `=======`, `>>>>>>>`)
3. Vyrieš, otestuj, `git add` vyriešené súbory
4. Pri **merge**: `git commit`; pri **rebase**: `git rebase --continue`
5. `git push`

## Undo / Obnova (opatrne)
```
git restore <súbor>          # vráť obsah k HEAD
git reset --soft HEAD~1      # vráť posledný commit, zmeny staged
git reset --mixed HEAD~1     # vráť commit, zmeny unstaged
git reset --hard HEAD~1      # vráť commit a zahoď zmeny (nevratné)
git revert <hash>            # nový commit, ktorý ruší starý
```

## Užitočné aliasy
```
git config --global alias.st "status -sb"
git config --global alias.lg "log --oneline --graph --decorate --all"
git config --global alias.co "checkout"
git config --global alias.br "branch"
git config --global alias.ci "commit"
git config --global alias.last "log -1 HEAD"
```

## .gitignore (C/C++ a buildy)
```
build/
bin/
*.o
*.a
*.so
*.dll
*.exe
*.dylib
.vscode/
*.code-workspace
.DS_Store
```

## .gitattributes (LF konce riadkov)
```
* text=auto
*.sh text eol=lf
*.c  text eol=lf
*.h  text eol=lf
Makefile text eol=lf
```

## SSH prístup
```
ssh-keygen -t ed25519 -C "tvoj@mail"
# verejný kľúč (~/.ssh/id_ed25519.pub) pridaj do GitHub/GitLab
ssh -T git@github.com
```

## Typické školské flow
```
git clone <url> && cd projekt
git switch -c uloha1
# práca... potom:
git add . && git commit -m "Implementácia verifikácie účtu"
git push -u origin uloha1
# po review:
git switch main && git pull && git merge uloha1 && git push
```
