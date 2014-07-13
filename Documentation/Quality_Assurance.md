Before each commit, ideally these tests are run with 'make qa'

./script/build.sh

./script/check.sh

make tests

make examples

scripts/development/mini.sh
scripts/development/medium.sh
scripts/development/medium-2.sh
scripts/development/run-argonnite-1-28.sh

shasum for the development tests:

mini sha1sum a2061b609b49f43290899da9d403517f352c1cf3
medium-2 sha1sum 8eeaf425d60732e90784c60debf38386f6b51519
medium sha1sum 8ea9b2795d93f2d92d6b29c910a2e517af1f6381
run-argonnite-1 sha1sum 71deaa88265222cd8c27c88980eb5c4f29c966af

S3 bucket with public stuff:

- s3://biosal
- http://biosal.s3.amazonaws.com/index.html
