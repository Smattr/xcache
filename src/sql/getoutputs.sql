select filename, timestamp, mode, uid, gid, contents from output where fk_operation = @fk_operation;
