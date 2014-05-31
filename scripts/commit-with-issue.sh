#!/bin/bash

# https://developer.github.com/v3/issues/#get-a-single-issue

if test $# -ne 1
then
    echo "Provide a an issue"
    exit
fi

owner=GeneAssembly
repo=biosal
worker=sebhtml
branch=energy
number=$1

# https://help.github.com/articles/creating-an-access-token-for-command-line-use
# https://developer.github.com/v3/auth/#basic-authentication
token=$(cat ~/github-token.txt)

# get title
title=$(curl -X GET https://api.github.com/repos/$owner/$repo/issues/$number | grep '"title": '|sed 's=  "title": "==g'|sed 's=",==g')
link=https://github.com/$owner/$repo/issues/$number

echo "Fetching issue"
echo "Title=$title"
echo "Link: $link"

(
cat << EOF
$title

Link: $link
EOF
) > message.txt

echo "Committing content"
git commit --all --signoff -F message.txt --edit
rm message.txt

echo "Pushing branch"
git push origin $branch &> /dev/null

commit=$(git log|head -n1 | awk '{print $2}')
commit_link=https://github.com/$worker/$repo/commit/$commit

# see http://stackoverflow.com/questions/7172784/how-to-post-json-data-with-curl-from-terminal-commandline-to-test-spring-rest
echo "Adding comment"
curl -X POST \
     -u $token:x-oauth-basic \
     -H "Content-Type: application/json" \
     -d "{\"body\": \"@hubot says: implemented in commit $commit_link by @$worker !\"}" \
     https://api.github.com/repos/$owner/$repo/issues/$number/comments &> /dev/null

echo "Closing issue"
curl -X POST \
     -u $token:x-oauth-basic \
     -H "Content-Type: application/json" \
     -d "{\"state\": \"closed\"}" \
     https://api.github.com/repos/$owner/$repo/issues/$number &> /dev/null
