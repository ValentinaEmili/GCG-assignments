git clone https://gitlab.tuwien.ac.at/e193-02-gcg-course/data.git
mkdir -p ../assets/
mkdir -p ../lib/
mkdir -p ../include/
cp -r ./data/assets/* ../assets/
cp -r ./data/lib/* ../lib/
cp -r ./data/include/* ../include/
rm -r data