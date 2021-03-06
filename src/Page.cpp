#include "Page.h"

int Page::_pid = 0;

pair<char*, int> pack(bool* arr, int boolLen);
bool* unpack(char* arr, int boolLen);
bool* readBoolArr(istream &in, int num);
void writeBoolArr(ostream &out, bool* data, int num);
void writeBinaryInt(ostream &out, int data);
int readBinaryInt(istream &in);

Page* Page::create(int itemNum, int itemSize) {
	auto p = new Page();
	p->pid = _pid++;
	p->bitmap = new bool[itemNum]{ 0 };
	p->data = new char[itemSize * itemNum]{ 0 };
	p->dirty = true;
	p->itemNum = itemNum;
	p->itemSize = itemSize;
	return p;
}

Page* Page::create(int itemSize) {
	return Page::create((PAGE_SIZE - 32) / (itemSize + 1), itemSize); // 32: safe zone
}

Page* Page::createFromFile(const string &path) {
	ifstream in(path, std::ios::binary);
	if (!in) {
		throw new exception(("open failed with path: " + path).c_str());
	}
	int pid, itemNum, itemSize;
	pid = readBinaryInt(in);
	itemSize = readBinaryInt(in);
	itemNum = readBinaryInt(in);
	auto p = create(itemNum, itemSize);
	p->itemSize = itemSize;
	p->itemNum = itemNum;
	p->pid = pid;
	_pid = max(_pid, pid);	// update max pid
	in.read((char*)p->bitmap, itemNum);
	in.read(p->data, itemSize * itemNum);
	in.close();
	p->dirty = false;

	return p;
}

Page::~Page() {
	delete[] data;
	delete[] bitmap;
}

void Page::writeBack() {
	if (!filepath.empty())
		writeBack(filepath);
}

void Page::writeBack(const string& filepath) {
	if (!dirty) return;
	ofstream out(filepath, std::ios::binary | std::ios::out);
	out.seekp(PAGE_SIZE - 1);
	out.put('\0');
	out.seekp(0);
	writeBinaryInt(out, pid);
	writeBinaryInt(out, itemSize);
	writeBinaryInt(out, itemNum);
	// writeBoolArr(out, bitmap, itemNum);
	out.write((char*)bitmap, itemNum);
	out.write(data, itemSize * itemNum);
	out.close();
	dirty = false;
}

const char* Page::getBySlot(int slotNum) {
	if (bitmap[slotNum]) return data + slotNum * itemSize;
	else return NULL;
}

void Page::setBySlot(int slotNum, char* data) {
	bitmap[slotNum] = true;
	memcpy(this->data + slotNum * itemSize, data, itemSize);
	dirty = true;
}

bool Page::isDirty() { return dirty; }

pair<char*, int> pack(bool* arr, int boolLen) {
	int len = boolLen / 8 + (boolLen % 8 ? 1 : 0);
	char* packed = new char[len] {0};
	int pos = 0, offset = 0;
	for (int i = 0; i < boolLen; i++) {
		if (arr[i]) {
			packed[pos] |= 0b10000000 >> offset;
		}
		if (++offset % 8 == 0) {
			pos++;
			offset = 0;
		}
	}

	return make_pair(packed, len);
}

bool* unpack(char* arr, int boolLen) {
	bool* unpacked = new bool[boolLen];

	int pos = 0, offset = 0;
	for (int i = 0; i < boolLen; i++) {
		if (++offset % 8) {
			pos++;
			offset = 0;
		}
		unpacked[i] = arr[pos] & (0b10000000 >> offset);
	}

	return unpacked;
}

bool* readBoolArr(istream &in, int num) {
	int len = num / 8 + (num % 8 ? 1 : 0);
	char* origin = new char[len];
	in.read(origin, len);
	bool* converted = unpack(origin, len);
	delete[] origin;
	return converted;
}

void writeBoolArr(ostream &out, bool* data, int num) {
	auto packed = pack(data, num);
	out.write(packed.first, packed.second);
	delete[] packed.first;
}

void writeBinaryInt(ostream &out, int data) {
	out.write((char*)&data, sizeof(data));
}

int readBinaryInt(istream &in) {
	int tmp;
	in.read((char*)&tmp, sizeof(int));
	return tmp;
}