local config = {}

function config.save(mod_name, filename, data, opts)
  opts = opts or {}
  minecraft.config.save(filename, data, {
    keys = opts.keys,
    names = opts.names,
    separator = opts.separator,
  })
end

function config.load(mod_name, filename, data, opts)
  opts = opts or {}
  minecraft.config.load(filename, data, {
    keys = opts.keys,
    names = opts.names,
    separator = opts.separator,
  })
end

return config
