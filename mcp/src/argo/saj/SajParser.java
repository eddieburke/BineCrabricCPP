/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.saj;

import argo.saj.InvalidSyntaxException;
import argo.saj.JsonListener;
import argo.saj.PositionTrackingPushbackReader;
import java.io.Reader;
import java.util.Arrays;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class SajParser {
    public void parse(Reader reader, JsonListener jsonListener) {
        PositionTrackingPushbackReader positionTrackingPushbackReader = new PositionTrackingPushbackReader(reader);
        char c = (char)positionTrackingPushbackReader.read();
        switch (c) {
            case '{': {
                positionTrackingPushbackReader.unread(c);
                jsonListener.startDocument();
                this.objectString(positionTrackingPushbackReader, jsonListener);
                break;
            }
            case '[': {
                positionTrackingPushbackReader.unread(c);
                jsonListener.startDocument();
                this.arrayString(positionTrackingPushbackReader, jsonListener);
                break;
            }
            default: {
                throw new InvalidSyntaxException("Expected either [ or { but got [" + c + "].", positionTrackingPushbackReader);
            }
        }
        int n = this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        if (n != -1) {
            throw new InvalidSyntaxException("Got unexpected trailing character [" + (char)n + "].", positionTrackingPushbackReader);
        }
        jsonListener.endDocument();
    }

    private void arrayString(PositionTrackingPushbackReader positionTrackingPushbackReader, JsonListener jsonListener) {
        char c = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        if (c != '[') {
            throw new InvalidSyntaxException("Expected object to start with [ but got [" + c + "].", positionTrackingPushbackReader);
        }
        jsonListener.startArray();
        char c2 = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        positionTrackingPushbackReader.unread(c2);
        if (c2 != ']') {
            this.aJsonValue(positionTrackingPushbackReader, jsonListener);
        }
        boolean bl = false;
        block4: while (!bl) {
            char c3 = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
            switch (c3) {
                case ',': {
                    this.aJsonValue(positionTrackingPushbackReader, jsonListener);
                    continue block4;
                }
                case ']': {
                    bl = true;
                    continue block4;
                }
            }
            throw new InvalidSyntaxException("Expected either , or ] but got [" + c3 + "].", positionTrackingPushbackReader);
        }
        jsonListener.endArray();
    }

    private void objectString(PositionTrackingPushbackReader positionTrackingPushbackReader, JsonListener jsonListener) {
        char c = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        if (c != '{') {
            throw new InvalidSyntaxException("Expected object to start with { but got [" + c + "].", positionTrackingPushbackReader);
        }
        jsonListener.startObject();
        char c2 = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        positionTrackingPushbackReader.unread(c2);
        if (c2 != '}') {
            this.aFieldToken(positionTrackingPushbackReader, jsonListener);
        }
        boolean bl = false;
        block4: while (!bl) {
            char c3 = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
            switch (c3) {
                case ',': {
                    this.aFieldToken(positionTrackingPushbackReader, jsonListener);
                    continue block4;
                }
                case '}': {
                    bl = true;
                    continue block4;
                }
            }
            throw new InvalidSyntaxException("Expected either , or } but got [" + c3 + "].", positionTrackingPushbackReader);
        }
        jsonListener.endObject();
    }

    private void aFieldToken(PositionTrackingPushbackReader positionTrackingPushbackReader, JsonListener jsonListener) {
        char c = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        if ('\"' != c) {
            throw new InvalidSyntaxException("Expected object identifier to begin with [\"] but got [" + c + "].", positionTrackingPushbackReader);
        }
        positionTrackingPushbackReader.unread(c);
        jsonListener.startField(this.stringToken(positionTrackingPushbackReader));
        char c2 = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        if (c2 != ':') {
            throw new InvalidSyntaxException("Expected object identifier to be followed by : but got [" + c2 + "].", positionTrackingPushbackReader);
        }
        this.aJsonValue(positionTrackingPushbackReader, jsonListener);
        jsonListener.endField();
    }

    private void aJsonValue(PositionTrackingPushbackReader positionTrackingPushbackReader, JsonListener jsonListener) {
        char c = (char)this.readNextNonWhitespaceChar(positionTrackingPushbackReader);
        switch (c) {
            case '\"': {
                positionTrackingPushbackReader.unread(c);
                jsonListener.stringValue(this.stringToken(positionTrackingPushbackReader));
                break;
            }
            case 't': {
                char[] cArray = new char[3];
                int n = positionTrackingPushbackReader.read(cArray);
                if (n != 3 || cArray[0] != 'r' || cArray[1] != 'u' || cArray[2] != 'e') {
                    positionTrackingPushbackReader.uncount(cArray);
                    throw new InvalidSyntaxException("Expected 't' to be followed by [[r, u, e]], but got [" + Arrays.toString(cArray) + "].", positionTrackingPushbackReader);
                }
                jsonListener.trueValue();
                break;
            }
            case 'f': {
                char[] cArray = new char[4];
                int n = positionTrackingPushbackReader.read(cArray);
                if (n != 4 || cArray[0] != 'a' || cArray[1] != 'l' || cArray[2] != 's' || cArray[3] != 'e') {
                    positionTrackingPushbackReader.uncount(cArray);
                    throw new InvalidSyntaxException("Expected 'f' to be followed by [[a, l, s, e]], but got [" + Arrays.toString(cArray) + "].", positionTrackingPushbackReader);
                }
                jsonListener.falseValue();
                break;
            }
            case 'n': {
                char[] cArray = new char[3];
                int n = positionTrackingPushbackReader.read(cArray);
                if (n != 3 || cArray[0] != 'u' || cArray[1] != 'l' || cArray[2] != 'l') {
                    positionTrackingPushbackReader.uncount(cArray);
                    throw new InvalidSyntaxException("Expected 'n' to be followed by [[u, l, l]], but got [" + Arrays.toString(cArray) + "].", positionTrackingPushbackReader);
                }
                jsonListener.nullValue();
                break;
            }
            case '-': 
            case '0': 
            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
            case '9': {
                positionTrackingPushbackReader.unread(c);
                jsonListener.numberValue(this.numberToken(positionTrackingPushbackReader));
                break;
            }
            case '{': {
                positionTrackingPushbackReader.unread(c);
                this.objectString(positionTrackingPushbackReader, jsonListener);
                break;
            }
            case '[': {
                positionTrackingPushbackReader.unread(c);
                this.arrayString(positionTrackingPushbackReader, jsonListener);
                break;
            }
            default: {
                throw new InvalidSyntaxException("Invalid character at start of value [" + c + "].", positionTrackingPushbackReader);
            }
        }
    }

    private String numberToken(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if ('-' == c) {
            stringBuilder.append('-');
        } else {
            positionTrackingPushbackReader.unread(c);
        }
        stringBuilder.append(this.nonNegativeNumberToken(positionTrackingPushbackReader));
        return stringBuilder.toString();
    }

    private String nonNegativeNumberToken(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if ('0' == c) {
            stringBuilder.append('0');
            stringBuilder.append(this.possibleFractionalComponent(positionTrackingPushbackReader));
            stringBuilder.append(this.possibleExponent(positionTrackingPushbackReader));
        } else {
            positionTrackingPushbackReader.unread(c);
            stringBuilder.append(this.nonZeroDigitToken(positionTrackingPushbackReader));
            stringBuilder.append(this.digitString(positionTrackingPushbackReader));
            stringBuilder.append(this.possibleFractionalComponent(positionTrackingPushbackReader));
            stringBuilder.append(this.possibleExponent(positionTrackingPushbackReader));
        }
        return stringBuilder.toString();
    }

    private char nonZeroDigitToken(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        char c;
        char c2 = (char)positionTrackingPushbackReader.read();
        switch (c2) {
            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
            case '9': {
                c = c2;
                break;
            }
            default: {
                throw new InvalidSyntaxException("Expected a digit 1 - 9 but got [" + c2 + "].", positionTrackingPushbackReader);
            }
        }
        return c;
    }

    private char digitToken(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        char c;
        char c2 = (char)positionTrackingPushbackReader.read();
        switch (c2) {
            case '0': 
            case '1': 
            case '2': 
            case '3': 
            case '4': 
            case '5': 
            case '6': 
            case '7': 
            case '8': 
            case '9': {
                c = c2;
                break;
            }
            default: {
                throw new InvalidSyntaxException("Expected a digit 1 - 9 but got [" + c2 + "].", positionTrackingPushbackReader);
            }
        }
        return c;
    }

    private String digitString(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        boolean bl = false;
        block3: while (!bl) {
            char c = (char)positionTrackingPushbackReader.read();
            switch (c) {
                case '0': 
                case '1': 
                case '2': 
                case '3': 
                case '4': 
                case '5': 
                case '6': 
                case '7': 
                case '8': 
                case '9': {
                    stringBuilder.append(c);
                    continue block3;
                }
            }
            bl = true;
            positionTrackingPushbackReader.unread(c);
        }
        return stringBuilder.toString();
    }

    private String possibleFractionalComponent(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if (c == '.') {
            stringBuilder.append('.');
            stringBuilder.append(this.digitToken(positionTrackingPushbackReader));
            stringBuilder.append(this.digitString(positionTrackingPushbackReader));
        } else {
            positionTrackingPushbackReader.unread(c);
        }
        return stringBuilder.toString();
    }

    private String possibleExponent(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if (c == '.' || c == 'E') {
            stringBuilder.append('E');
            stringBuilder.append(this.possibleSign(positionTrackingPushbackReader));
            stringBuilder.append(this.digitToken(positionTrackingPushbackReader));
            stringBuilder.append(this.digitString(positionTrackingPushbackReader));
        } else {
            positionTrackingPushbackReader.unread(c);
        }
        return stringBuilder.toString();
    }

    private String possibleSign(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if (c == '+' || c == '-') {
            stringBuilder.append(c);
        } else {
            positionTrackingPushbackReader.unread(c);
        }
        return stringBuilder.toString();
    }

    private String stringToken(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        StringBuilder stringBuilder = new StringBuilder();
        char c = (char)positionTrackingPushbackReader.read();
        if ('\"' != c) {
            throw new InvalidSyntaxException("Expected [\"] but got [" + c + "].", positionTrackingPushbackReader);
        }
        boolean bl = false;
        block4: while (!bl) {
            char c2 = (char)positionTrackingPushbackReader.read();
            switch (c2) {
                case '\"': {
                    bl = true;
                    continue block4;
                }
                case '\\': {
                    char c3 = this.escapedStringChar(positionTrackingPushbackReader);
                    stringBuilder.append(c3);
                    continue block4;
                }
            }
            stringBuilder.append(c2);
        }
        return stringBuilder.toString();
    }

    private char escapedStringChar(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        char c;
        char c2 = (char)positionTrackingPushbackReader.read();
        switch (c2) {
            case '\"': {
                c = '\"';
                break;
            }
            case '\\': {
                c = '\\';
                break;
            }
            case '/': {
                c = '/';
                break;
            }
            case 'b': {
                c = '\b';
                break;
            }
            case 'f': {
                c = '\f';
                break;
            }
            case 'n': {
                c = '\n';
                break;
            }
            case 'r': {
                c = '\r';
                break;
            }
            case 't': {
                c = '\t';
                break;
            }
            case 'u': {
                c = (char)this.hexidecimalNumber(positionTrackingPushbackReader);
                break;
            }
            default: {
                throw new InvalidSyntaxException("Unrecognised escape character [" + c2 + "].", positionTrackingPushbackReader);
            }
        }
        return c;
    }

    private int hexidecimalNumber(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        int n;
        char[] cArray = new char[4];
        int n2 = positionTrackingPushbackReader.read(cArray);
        if (n2 != 4) {
            throw new InvalidSyntaxException("Expected a 4 digit hexidecimal number but got only [" + n2 + "], namely [" + String.valueOf(cArray, 0, n2) + "].", positionTrackingPushbackReader);
        }
        try {
            n = Integer.parseInt(String.valueOf(cArray), 16);
        }
        catch (NumberFormatException numberFormatException) {
            positionTrackingPushbackReader.uncount(cArray);
            throw new InvalidSyntaxException("Unable to parse [" + String.valueOf(cArray) + "] as a hexidecimal number.", numberFormatException, positionTrackingPushbackReader);
        }
        return n;
    }

    private int readNextNonWhitespaceChar(PositionTrackingPushbackReader positionTrackingPushbackReader) {
        int n;
        boolean bl = false;
        do {
            n = positionTrackingPushbackReader.read();
            switch (n) {
                case 9: 
                case 10: 
                case 13: 
                case 32: {
                    break;
                }
                default: {
                    bl = true;
                }
            }
        } while (!bl);
        return n;
    }
}

