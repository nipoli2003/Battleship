# Contributing
## Branches
| Branch           | Purpose       | Who writes                           |
|------------------|---------------|--------------------------------------|
| `main`           | Releases only | Team lead, via release PR            |
| `development`    | Integration   | **Nobody directly**, PR merges only  |
| `<name>/<thing>` | One task      | You                                  |

NOTE: `development` is protected, so you should not be able to push to it

## Workflow
1. **Sync** first
```bash
git checkout development && git pull origin development
```
2. **Open an issue** for everything non trivial. One issue = one feature/bug
3. **Claim the issue**: Assign yourself to the issue, so that everyone knows who is working on it
4. **Branch** off `development`
```bash
git checkout -b <yourname/short-description>
```
One branch per issue. Don't bundle unrelated work.

5. **Work, then push your branch**. You can (should!) push your branch even if you are not finished, so that other people see your progress
```bash
git push -u origin <your_branch_name>
```
6. **Create a Pull Request** into `development`. Request 1-2 approvals, and mention the issue you were working on (e.g. `Closes #N`). Note that you can also create "Draft PR", that
7. **Review** that the code makes sense and adheres to the SDS
8. **Merge**: Once the CI build and CI test succeed and the approval is given, merge the PR.

## Push rejected / conflicts
```bash
git pull --rebase origin <branch>                  # push rejected
git fetch origin && git merge origin/development   # conflict with development
```
Never `--force` over work you didn't write!

## Stuck?
1. try rebuilding `rm -rf build && cmake -B build`
2. ask in the team chat
