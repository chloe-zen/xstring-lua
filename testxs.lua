-- Lua portion of xstring self-test

require "xstring"

function testxs (wool)
    assert(type(wool) == "userdata")
    print("wool = '" .. tostring(wool) .. "'");

    --local w = tostring(wool)
    --print("type(w) = " .. type(w));
    --print("#w = " .. #w);
    assert(tostring(wool) == "Pull the wool over your own eyes!");

    -- test object-oriented usage and xstring.substr()
    local w = wool:sub(10, 13)
    print("sub = '" .. tostring(w) .. "'")
    assert(tostring(w) == "wool")

    -- call test-program-defined function
    local pos = xfindbyte(wool, 'v')
    print("xfindbyte('v') = " .. pos)
    assert(pos == 16)
end
