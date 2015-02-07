PROJECT="libqtxdg"
version="$1"
shift

if [[ -z $version ]]; then
	>&2 echo "USAGE: $0 <tag>"
	exit 1
fi

mkdir -p "dist/$version"
echo "Creating $PROJECT.tar.gz"
git archive -9 --format tar.gz $version --prefix="$PROJECT/" > "dist/$version/$PROJECT.tar.gz"
gpg --armor --detach-sign "dist/$version/$PROJECT.tar.gz"
echo "Creating $PROJECT.tar.xz"
git archive -9 --format tar.xz $version --prefix="$PROJECT/" > "dist/$version/$PROJECT.tar.xz"
gpg --armor --detach-sign "dist/$version/$PROJECT.tar.xz"
cd "dist/$version"

sha1sum --tag *.tar.gz *.tar.xz >> CHECKSUMS
sha256sum --tag *.tar.gz *.tar.xz >> CHECKSUMS

cd ..
echo "Uploading to lxqt.org..."

scp -r "$version" "lxde.org:/var/www/lxqt/downloads/$PROJECT/"
