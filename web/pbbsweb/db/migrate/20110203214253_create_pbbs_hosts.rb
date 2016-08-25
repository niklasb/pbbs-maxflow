class CreatePbbsHosts < ActiveRecord::Migration
  def self.up
    create_table :pbbs_hosts do |t|

      t.timestamps
    end
  end

  def self.down
    drop_table :pbbs_hosts
  end
end
